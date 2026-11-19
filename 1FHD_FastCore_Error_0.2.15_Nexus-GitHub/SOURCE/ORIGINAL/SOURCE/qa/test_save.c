#include "../save_recovery.h"

typedef struct {
    Word32 bytes;
    Wide value[48];
} TestKey;

typedef struct {
    Byte prefix[0x38];
    const Wide *key;
    Byte equip_data[0x2c];
    Byte counts_as, shared_use, shared_effect, marker;
    int data[3];
    const Wide *text[3];
} Artefact;

typedef struct {
    int data[3], integer_position;
    Byte flags[3];
    int boolean_position, wide_position;
    const Wide *text[3];
} Buffer;

extern void fail_test(int code) __attribute__((noreturn));
extern void run_artefact_body(Artefact *item, Buffer *buffer);
extern const Wide *run_artefact_bitmap(Artefact *item);
extern void take_artefact_bitmap_key(void);
extern int run_extra_module(Buffer *buffer, int module);
extern void take_extra_module(void);
extern void skip_extra_module(void);

static TestKey missing = {14, L"Missing"};
static TestKey unicode = {12, L"Талмир"};
static TestKey real = {8, L"Real"};
static TestKey invalid = {14, L"Foo.Bar"};
static TestKey colliding = {12, L"String"};
static TestKey native_icon = {8, L"Hull"};
static void *root = (void *)0x410000u;
static void *arts = (void *)0x411000u;
static void *custom = (void *)0x412000u;
static void *real_block = (void *)0x413000u;
static struct {
    const Wide *key;
    unsigned int params;
} stubs[4];
static int config_calls, original_restored;
static unsigned int stub_count;

static int equal(const Wide *a, const Wide *b)
{
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return *a == *b;
}

static void DELPHI native_config(Artefact *item, Byte weight)
{
    if (weight)
        fail_test(60);
    ++config_calls;
    item->counts_as = 21;
    item->shared_use = 1;
    item->shared_effect = 1;
}

static void *DELPHI find_block(void *parent, const Wide *key)
{
    if (parent == root && equal(key, L"Artefacts"))
        return arts;
    if (parent == arts && equal(key, L"CustomArtefacts"))
        return custom;
    if (parent == custom) {
        if (equal(key, real.value) || (original_restored && equal(key, missing.value)))
            return real_block;
        for (unsigned int i = 0; i < stub_count; ++i)
            if (equal(key, stubs[i].key))
                return &stubs[i];
    }
    return 0;
}

static int DELPHI count_params(void *block, const Wide *key)
{
    if (block == custom)
        return equal(key, colliding.value);
    if (!equal(key, L"FHD_FastCore_Error_MissingDefinition"))
        fail_test(61);
    for (unsigned int i = 0; i < stub_count; ++i)
        if (block == &stubs[i])
            return stubs[i].params == 255;
    return 0;
}

static void *DELPHI add_block(void *block, const Wide *key)
{
    if (block != custom || stub_count >= 4)
        fail_test(62);
    stubs[stub_count].key = key;
    return &stubs[stub_count++];
}

static void *DELPHI add_param(void *block, const Wide *key, const Wide *value)
{
    static const Wide *const keys[] = {
        L"Name", L"Text", L"Description", L"Broken", L"BrokenInBattle",
        L"BrokenInUse", L"BrokenByForce", L"FHD_FastCore_Error_MissingDefinition"
    };
    if (!stub_count || block != &stubs[stub_count - 1])
        fail_test(63);
    unsigned int i = 0;
    while (i < 8 && !equal(key, keys[i]))
        ++i;
    if (i == 8 || !*value || (stubs[stub_count - 1].params & (1u << i)) ||
        (i == 0 && value != stubs[stub_count - 1].key) ||
        (i == 7 && !equal(value, L"1")))
        fail_test(64);
    stubs[stub_count - 1].params |= 1u << i;
    return 0;
}

static int DELPHI read_int(Buffer *buffer)
{
    if (buffer->integer_position >= 3)
        fail_test(65);
    return buffer->data[buffer->integer_position++];
}

static Byte DELPHI read_bool(Buffer *buffer)
{
    if (buffer->boolean_position >= 3)
        fail_test(66);
    return buffer->flags[buffer->boolean_position++];
}

static void DELPHI read_wide(Buffer *buffer, const Wide **out)
{
    if (buffer->wide_position >= 3)
        fail_test(67);
    *out = buffer->text[buffer->wide_position++];
}

static void DELPHI copy_wide(const Wide **out, const Wide *value)
{
    *out = value;
}

static void branch(Word32 address, Word32 target)
{
    volatile Byte *at = (volatile Byte *)address;
    Word32 relative = target - address - 5;
    at[0] = 0xe9;
    for (unsigned int i = 0; i < 4; ++i)
        at[i + 1] = (Byte)(relative >> (i * 8));
}

static void test_artefact(const Wide *key, int disk_load, int make_placeholder)
{
    Buffer buffer = {.data = {47, -15, 42}, .flags = {1, 0, 1},
                     .text = {L"first", L"third"}};
    Artefact item = {.key = key, .equip_data = {2}};
    item.prefix[8] = 93; // Saved object ID.
    item.prefix[0x0c] = 9; // Native custom artefact type with no wear.
    *(void **)0x81cc70u = disk_load ? &buffer : 0;
    const int before = config_calls;
    run_artefact_body(&item, &buffer);
    if (item.key != key || item.prefix[8] != 93 || item.prefix[0x0c] != 9 ||
        item.equip_data[0] != 2 ||
        item.data[0] != 47 || item.data[1] != -15 || item.data[2] != 42 ||
        !equal(item.text[0], L"first") || item.text[1] || !equal(item.text[2], L"third") ||
        buffer.integer_position != 3 || buffer.boolean_position != 3 || buffer.wide_position != 2)
        fail_test(68);

    if (make_placeholder) {
        if (config_calls != before || item.counts_as != 8 || item.shared_use || item.shared_effect ||
            run_artefact_bitmap(&item) != native_icon.value)
            fail_test(69);
    } else if (config_calls != before + 1 || item.counts_as != 21 || !item.shared_use ||
               !item.shared_effect || run_artefact_bitmap(&item) != key) {
        fail_test(70);
    }
}

void test_save_recovery(void)
{
    static const Wide *item_names[11] = {[10] = native_icon.value};
    Buffer context = {0};
    int save_version = 164;
    *(int **)0x813580u = &save_version;
    *(void **)0x81cfc0u = root;
    *(const Wide ***)0x8131e0u = (const Wide **)item_names;
    branch(0x70f4f4, (Word32)native_config);
    branch(0x7b3008, (Word32)find_block);
    branch(0x7b2ad8, (Word32)count_params);
    branch(0x7b2ef0, (Word32)add_block);
    branch(0x7b2640, (Word32)add_param);
    branch(0x7b82d0, (Word32)read_int);
    branch(0x7b8424, (Word32)read_bool);
    branch(0x7b8750, (Word32)read_wide);
    branch(0x405d54, (Word32)copy_wide);
    branch(0x70f8d2, 0x70f8f4);
    branch(0x70e736, (Word32)take_artefact_bitmap_key);
    branch(0x6f76af, (Word32)take_extra_module);
    branch(0x6f775d, (Word32)skip_extra_module);

    test_artefact(missing.value, 0, 0);
    test_artefact(real.value, 1, 0);
    test_artefact(invalid.value, 1, 0);
    test_artefact(colliding.value, 1, 0);
    test_artefact(missing.value, 1, 1);
    test_artefact(unicode.value, 1, 1);
    test_artefact(missing.value, 1, 1);
    test_artefact(unicode.value, 0, 1);
    test_artefact(real.value, 0, 0);
    if (stub_count != 2 || stubs[0].params != 255 || stubs[1].params != 255)
        fail_test(71);
    // The original mod returns with a fresh configuration tree.
    original_restored = 1;
    root = (void *)0x415000u;
    *(void **)0x81cfc0u = root;
    test_artefact(missing.value, 1, 0);

    context.data[0] = 3;
    *(void **)0x81cc70u = &context;
    if (run_extra_module(&context, 7) != 7 || context.integer_position != 0 ||
        run_extra_module(&context, 0) != 0 || context.integer_position != 1)
        fail_test(72);
    save_version = 156;
    if (run_extra_module(&context, 0) != 1 || context.integer_position != 1)
        fail_test(73);
    save_version = 164;
    *(void **)0x81cc70u = 0;
    if (run_extra_module(&context, 0) != 1 || context.integer_position != 1)
        fail_test(74);
}
