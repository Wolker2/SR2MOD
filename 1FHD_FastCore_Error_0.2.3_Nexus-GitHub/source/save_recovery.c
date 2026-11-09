#include "save_recovery.h"

#define BSTR_LITERAL(name, literal) \
    static const struct { Word32 byte_length; Wide text[sizeof(literal) / sizeof(Wide)]; } \
        name = {sizeof(literal) - sizeof(Wide), literal}

BSTR_LITERAL(artefacts, L"Artefacts");
BSTR_LITERAL(custom_artefacts, L"CustomArtefacts");
BSTR_LITERAL(placeholder, L"FHD_FastCore_Error_MissingDefinition");
BSTR_LITERAL(one, L"1");
BSTR_LITERAL(name_param, L"Name");
BSTR_LITERAL(text_param, L"Text");
BSTR_LITERAL(description_param, L"Description");
BSTR_LITERAL(broken, L"Broken");
BSTR_LITERAL(broken_in_battle, L"BrokenInBattle");
BSTR_LITERAL(broken_in_use, L"BrokenInUse");
BSTR_LITERAL(broken_by_force, L"BrokenByForce");
BSTR_LITERAL(missing_description, L"Mod data unavailable. This artefact has no effect.");
BSTR_LITERAL(prefix, L"Save recovery: missing artefact definition ");

enum { max_key_length = 255, placeholder_flag = 0xa7 };
typedef struct {
    Byte prefix[0x0c];
    Byte item_type;
    Byte before_key[0x2b];
    const Wide *config_key;
    Byte before_custom_data[0x2c];
    Byte counts_as;
    Byte shared_use;
    Byte shared_effect;
    Byte recovery_flag; // Padding between Delphi's three bytes and Data[1].
} CustomArtefact;

_Static_assert(__builtin_offsetof(CustomArtefact, config_key) == 0x38, "ConfigBlockName");
_Static_assert(__builtin_offsetof(CustomArtefact, counts_as) == 0x68, "CountsAsItemType");
_Static_assert(__builtin_offsetof(CustomArtefact, recovery_flag) == 0x6b, "Padding before Data");

typedef void (DELPHI *LoadConfig)(CustomArtefact *, Byte);
typedef void *(DELPHI *BlockByName)(void *, const Wide *);
typedef int (DELPHI *CountParams)(void *, const Wide *);
typedef void *(DELPHI *AddParam)(void *, const Wide *, const Wide *);

static const LoadConfig load_config = (LoadConfig)0x70f4f4;
static const BlockByName find_block = (BlockByName)0x7b3008;
static const BlockByName add_block = (BlockByName)0x7b2ef0;
static const CountParams count_params = (CountParams)0x7b2ad8;
static const AddParam add_param = (AddParam)0x7b2640;

static const Word32 bitmap_resume __attribute__((used)) = 0x70e736;
static const Word32 extra_resume __attribute__((used)) = 0x6f76af;
static const Word32 extra_next __attribute__((used)) = 0x6f775d;
static const Word32 read_int __attribute__((used)) = 0x7b82d0;
static VisualLog save_log;
static void *placeholder_root;

void set_save_recovery_log(VisualLog log)
{
    save_log = log;
}

static unsigned int key_length(const Wide *key)
{
    if (!key)
        return 0;
    const Word32 bytes = ((const Word32 *)key)[-1];
    if ((bytes & 1) || bytes > max_key_length * sizeof(Wide))
        return 0;
    const unsigned int length = bytes / sizeof(Wide);
    for (unsigned int i = 0; i < length; ++i) {
        const Wide c = key[i];
        if (c < 0x20 || c == '.' || c == '/' || c == '\\' || c == ':')
            return 0;
    }
    return length;
}

static void log_new_placeholder(const Wide *key, unsigned int length)
{
    if (!save_log)
        return;
    enum { prefix_length = sizeof(prefix.text) / sizeof(Wide) - 1 };
    Wide message[prefix_length + max_key_length + 1];
    for (unsigned int i = 0; i < prefix_length; ++i)
        message[i] = prefix.text[i];
    for (unsigned int i = 0; i < length; ++i)
        message[prefix_length + i] = key[i];
    message[prefix_length + length] = 0;
    save_log(L"FHD_FastCore_Error", message);
}

static int prepare_placeholder(const Wide *key, int can_create)
{
    void *root = *(void *volatile *)0x81cfc0;
    if (!root || (!can_create && root != placeholder_root))
        return 0;
    const unsigned int length = key_length(key);
    if (!length)
        return 0;
    void *parent = find_block(root, artefacts.text);
    if (parent)
        parent = find_block(parent, custom_artefacts.text);
    if (!parent)
        return 0;

    void *block = find_block(parent, key);
    if (block)
        return count_params(block, placeholder.text) != 0;
    // Refuse a path occupied by a string parameter.
    if (!can_create || count_params(parent, key))
        return 0;
    block = add_block(parent, key);
    add_param(block, name_param.text, key);
    add_param(block, text_param.text, missing_description.text);
    add_param(block, description_param.text, missing_description.text);
    add_param(block, broken.text, missing_description.text);
    add_param(block, broken_in_battle.text, missing_description.text);
    add_param(block, broken_in_use.text, missing_description.text);
    add_param(block, broken_by_force.text, missing_description.text);
    add_param(block, placeholder.text, one.text);
    placeholder_root = root;
    log_new_placeholder(key, length);
    return 1;
}

static void DELPHI __attribute__((used)) configure_saved_artefact(
    CustomArtefact *item, Byte apply_weight, const void *buffer)
{
    // The same deserializer is also used outside loads of an on-disk game.
    const int disk_load = buffer && buffer == *(void *volatile *)0x81cc70;
    if ((item->item_type == 8 || item->item_type == 9) &&
        prepare_placeholder(item->config_key, disk_load)) {
        item->counts_as = 8;
        item->shared_use = 0;
        item->shared_effect = 0;
        item->recovery_flag = placeholder_flag;
        return;
    }
    item->recovery_flag = 0;
    load_config(item, apply_weight);
}

void __attribute__((naked)) artefact_config_entry(void)
{
    __asm__ volatile("mov %esi, %ecx\n jmp configure_saved_artefact");
}

void __attribute__((naked)) artefact_bitmap_entry(void)
{
    // Still use the game's native GI prefix, suffix, concatenation and result ABI.
    // The native item-name table entry at index 10 is the standard hull artefact.
    __asm__ volatile(
        "cmpb $0xa7, 0x6b(%ebx)\n"
        "jne 1f\n"
        "mov 0x8131e0, %eax\n"
        "pushl 40(%eax)\n"
        "jmp 2f\n"
        "1: pushl 0x38(%ebx)\n"
        "2: pushl $0x70e7e4\n"
        "jmp *bitmap_resume\n"
    );
}

void __attribute__((naked)) missing_extra_module_entry(void)
{
    // This splice is reached only after native template resolution returned zero.
    // Modern saves always store an Int32 count even for the missing template.
    __asm__ volatile(
        "cmp %esi, 0x81cc70\n"
        "jne 1f\n"
        "test %esi, %esi\n"
        "jz 1f\n"
        "mov 0x813580, %eax\n"
        "cmpl $157, (%eax)\n"
        "jl 1f\n"
        "mov %esi, %eax\n"
        "call *read_int\n"
        "jmp *extra_next\n"
        "1: mov $1, %edi\n"
        "jmp *extra_resume\n"
    );
}
