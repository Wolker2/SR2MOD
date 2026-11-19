#include "orphan_cleanup.h"

enum { preview = 1, apply = 2, custom_weapon = 68, first_custom_award = 48,
       countable_misc = 75, path_capacity = 192, report_limit = 150 };

typedef struct {
    Word32 bytes;
    Wide text[path_capacity];
} Path;

typedef struct {
    void *vmt;
    void **items;
    int count, capacity;
} List;

typedef struct {
    void *vmt;
    const Wide *form, *control, *image, *original;
} ImageOverride;

typedef void *(DELPHI *FindBlock)(void *, const Wide *);
typedef Byte (DELPHI *FileExists)(void *, const Wide *);
typedef void (DELPHI *Delete)(List *, int);
typedef void (DELPHI *Free)(void *);
typedef void (DELPHI *AssignWide)(const Wide **, const Wide *);
typedef void (DELPHI *ReadWide)(void *, const Wide **);
typedef void (DELPHI *LoadImage)(ImageOverride *, void *);
typedef void (DELPHI *ReapplyImage)(ImageOverride *);
typedef int (DELPHI *IndexOf)(List *, const void *);
typedef void (DELPHI *ClearItemReferences)(void *, const void *);
typedef void (DELPHI *FilmCommand)(void *, int, void *);
typedef int (DELPHI *GetUnitSize)(const Byte *);
typedef void (*CompletePickup)(const Byte *);

static const FindBlock find_block = (FindBlock)0x7b2eb8;
static const FileExists file_exists = (FileExists)0x4d1c28;
static const Delete delete_entry = (Delete)0x41620c;
static const Free free_object = (Free)0x4045ac;
static const AssignWide assign_wide = (AssignWide)0x405d54;
static const ReadWide read_wide = (ReadWide)0x7b8750;
static const LoadImage load_image = (LoadImage)0x800250;
static const ReapplyImage reapply_image = (ReapplyImage)0x8002f0;
static const IndexOf index_of = (IndexOf)0x4163c4;
static const ClearItemReferences clear_item_references = (ClearItemReferences)0x7e145c;
static const FilmCommand detach_object = (FilmCommand)0x783ca4;
static const FilmCommand release_object = (FilmCommand)0x783ce0;
static const GetUnitSize get_unit_size = (GetUnitSize)0x707680;
static const CompletePickup complete_pickup = (CompletePickup)0x7f4f5c;
static const Word32 resolve_stars_resume __attribute__((used)) = 0x7d2e0e;
static const Word32 combat_resume __attribute__((used)) = 0x7f98d9;
static const Word32 combat_continue __attribute__((used)) = 0x7fa075;
static const Word32 open_missiles_resume __attribute__((used)) = 0x7e261f;
static const Word32 refresh_missiles_resume __attribute__((used)) = 0x7e27c7;
static const Word32 countable_block_resume __attribute__((used)) = 0x7076c3;
static const Word32 countable_size_finish __attribute__((used)) = 0x7076ee;
static const Word32 countable_merge_resume __attribute__((used)) = 0x7079d2;
static int cleanup_mode;
static VisualLog cleanup_log;
static unsigned int report_count;

void set_orphan_cleanup(int mode, VisualLog log)
{
    cleanup_mode = mode;
    cleanup_log = log;
}

static int append(Path *path, int used, const Wide *text)
{
    while (*text && used < path_capacity - 1)
        path->text[used++] = *text++;
    path->text[used] = 0;
    path->bytes = (Word32)used * sizeof(Wide);
    return *text ? -1 : used;
}

static void append_number(Path *path, int used, unsigned int value)
{
    Wide digits[11];
    int start = 10;
    digits[start] = 0;
    do {
        digits[--start] = (Wide)('0' + value % 10);
        value /= 10;
    } while (value);
    append(path, used, digits + start);
}

static int prefix_length(const Wide *value, const Wide *prefix)
{
    int length = 0;
    if (value)
        while (prefix[length] && value[length] == prefix[length])
            ++length;
    return prefix[length] ? 0 : length;
}

static int valid_list(const List *list)
{
    return list && list->count >= 0 && list->count <= list->capacity &&
           (!list->count || list->items);
}

static List *list_at(const Byte *object, unsigned int offset)
{
    return *(List *const *)(object + offset);
}

static void report(const Wide *kind, const Wide *key)
{
    if (!cleanup_log || __atomic_fetch_add(&report_count, 1, __ATOMIC_RELAXED) >= report_limit)
        return;
    Path line;
    int length = append(&line, 0, cleanup_mode == apply ? L"Cleanup removed: " : L"Cleanup candidate: ");
    length = append(&line, length, kind);
    append(&line, length, key);
    cleanup_log(L"FHD_FastCore_Error", line.text);
}

static void report_blocked_shot(const Wide *key)
{
    if (!cleanup_log || __atomic_fetch_add(&report_count, 1, __ATOMIC_RELAXED) >= report_limit)
        return;
    Path line;
    const int length = append(&line, 0, L"Cleanup blocked shot: ");
    append(&line, length, key);
    cleanup_log(L"FHD_FastCore_Error", line.text);
}

static void report_total(const Wide *kind, unsigned int count)
{
    if (!cleanup_log)
        return;
    Path line;
    int length = append(&line, 0, cleanup_mode == apply ? L"Cleanup removed " : L"Cleanup candidates ");
    length = append(&line, length, kind);
    append_number(&line, length, count);
    cleanup_log(L"FHD_FastCore_Error", line.text);
}

static int missing_config_block(void *config, const Wide *prefix, const Wide *name, Path *key)
{
    if (!config || !name || !*name)
        return 0;
    const int length = append(key, 0, prefix);
    return append(key, length, name) >= 0 && !find_block(config, key->text);
}

static const Wide *missile_suffix(const Byte *info, int special_module)
{
    if (special_module > 0) {
        // Delphi dynamic array: the element count is the word before the data.
        const Byte *templates = **(const Byte ***)0x8136b4;
        if (templates && special_module <= ((const int *)templates)[-1]) {
            const Wide *override = *(const Wide *const *)(templates +
                (special_module - 1) * 0x124 + 0x110);
            if (override && *override)
                return override;
        }
    }
    return info ? *(const Wide *const *)(info + 4) : 0;
}

static int missing_missile(void *config, const Byte *missile, Path *key)
{
    if (!missile || *(const void *const *)(missile + 4))
        return 0;
    const void *vmt = *(const void *const *)missile;
    const Byte *info = 0;
    if (vmt == *(const void **)0x5cd678)
        info = *(const Byte *const *)(missile + 0x74);
    else if (vmt != *(const void **)0x5cd610)
        return 0;
    const int special = *(const int *)(missile + 0x20);
    return missing_config_block(config, L"SE.Missile.w", missile_suffix(info, special), key);
}

static List *DELPHI __attribute__((used, noinline)) cleanup_live_missiles(Byte *star)
{
    List *missiles = list_at(star, 0x38);
    void *config = *(void **)0x81cfd8;
    if (!config || !valid_list(missiles))
        return missiles;
    for (int i = missiles->count - 1; i >= 0; --i) {
        Byte *missile = missiles->items[i];
        Path key;
        if (missile && *(void **)(missile + 0x38) == star &&
            !*(void **)(missile + 0x5c) && missing_missile(config, missile, &key)) {
            // The native destructor clears references and removes this live missile
            // from the star. This pass runs before the game takes the list count.
            free_object(missile);
            report(L"live missile ", key.text);
        }
    }
    return missiles;
}

void __attribute__((naked)) cleanup_missiles_open_scene_entry(void)
{
    __asm__ volatile(
        "mov -4(%ebp), %eax\n"
        "call cleanup_live_missiles\n"
        "jmp *open_missiles_resume\n"
    );
}

void __attribute__((naked)) cleanup_missiles_refresh_scene_entry(void)
{
    __asm__ volatile(
        "mov (%esp), %eax\n"
        "call cleanup_live_missiles\n"
        "jmp *refresh_missiles_resume\n"
    );
}

static int missing_countable(const Byte *item, Path *key)
{
    if (item[0x0c] != countable_misc)
        return 0;
    return missing_config_block(*(void **)0x81cfc0, L"Items.CustomCountables.",
                                *(const Wide *const *)(item + 0x38), key);
}

static void *DELPHI __attribute__((used, noinline)) find_countable_block(void *root, const Wide *key)
{
    return root ? find_block(root, key) : 0;
}

static int DELPHI __attribute__((used, noinline)) saved_countable_unit_size(const Byte *item)
{
    const int count = *(const int *)(item + 0x60);
    const int weight = *(const int *)(item + 0x18);
    return count > 0 && weight >= count ? weight / count : 1;
}

static Word32 DELPHI __attribute__((used, noinline)) countable_merged_weight(
    const Byte *item, const Byte *other)
{
    Path key;
    if (missing_countable(item, &key)) {
        const Word32 weight = *(const Word32 *)(item + 0x18);
        const Word32 other_weight = *(const Word32 *)(other + 0x18);
        if (weight <= 0x7fffffffu && other_weight <= 0x7fffffffu)
            return weight > 0x7fffffffu - other_weight ? 0x7fffffffu : weight + other_weight;
    }
    return (Word32)get_unit_size(item) * *(const Word32 *)(item + 0x60);
}

void __attribute__((naked)) countable_unit_size_entry(void)
{
    __asm__ volatile(
        "call find_countable_block\n"
        "test %eax, %eax\n"
        "jnz 1f\n"
        "mov %ebx, %eax\n"
        "call saved_countable_unit_size\n"
        "mov %eax, %esi\n"
        "jmp *countable_size_finish\n"
        "1: jmp *countable_block_resume\n"
    );
}

void __attribute__((naked)) countable_merge_weight_entry(void)
{
    __asm__ volatile(
        "mov %esi, %edx\n"
        "call countable_merged_weight\n"
        "jmp *countable_merge_resume\n"
    );
}

void guarded_countable_pickup(const Byte *frame)
{
    Byte *item = *(Byte *const *)(frame - 4);
    Path key;
    if (item && *(const int *)(item + 0x30) <= 0 && !item[0x34] &&
        !*(void *const *)(item + 0x2c) && missing_countable(item, &key)) {
        Byte *star = *(Byte *const *)(frame - 12);
        List *items = star ? list_at(star, 0x30) : 0;
        const int index = valid_list(items) ? index_of(items, item) : -1;
        if (index >= 0) {
            void *film = *(void **)0x81cc60;
            void *object = *(void **)(item + 0x28);
            if (frame[-13] && film && object) {
                const int step = *(const int *)(frame - 0x1c);
                detach_object(film, step, object);
                release_object(film, step, object);
            }
            delete_entry(items, index);
            clear_item_references(star, item);
            free_object(item);
            report(L"loose item stack ", key.text);
            return;
        }
    }
    complete_pickup(frame);
}

static unsigned int cleanup_missiles(Byte *star, void *config)
{
    List *missiles = list_at(star, 0x38);
    if (!valid_list(missiles))
        return 0;
    unsigned int count = 0;
    for (int i = missiles->count - 1; i >= 0; --i) {
        Byte *missile = missiles->items[i];
        Path key;
        if (!missing_missile(config, missile, &key))
            continue;
        ++count;
        report(L"missile ", key.text);
        if (cleanup_mode == apply) {
            // Before reference resolution CurrentStar is still a numeric ID.
            // Remove the entry from this list, then destroy it with no live star.
            delete_entry(missiles, i);
            *(void **)(missile + 0x38) = 0;
            free_object(missile);
        }
    }
    return count;
}

static int orphan_award(Word32 id, void *language, void *resources, Path *image)
{
    if (id < first_custom_award || id >= 255)
        return 0;
    Path text;
    int length = append(&text, 0, L"Reward.");
    append_number(&text, length, id);
    length = (int)(text.bytes / sizeof(Wide));
    append(&text, length, L".Name");
    if (find_config_parameter(language, text.text))
        return 0;
    append(&text, length, L".Text");
    if (find_config_parameter(language, text.text))
        return 0;
    length = append(image, 0, L"Bm.FormRewards.2_");
    append_number(image, length, id);
    return !file_exists(resources, image->text);
}

static unsigned int cleanup_awards(Byte *star, void *language, void *resources)
{
    List *ships = list_at(star, 0x2c);
    if (!language || !resources || !valid_list(ships))
        return 0;
    unsigned int count = 0;
    for (int i = 0; i < ships->count; ++i) {
        Byte *ship = ships->items[i];
        List *awards = ship ? list_at(ship, 0x3e8) : 0;
        if (!valid_list(awards))
            continue;
        int *visible = (int *)(ship + 0x3ec);
        for (int j = awards->count - 1; j >= 0; --j) {
            Path key;
            if (!orphan_award((Word32)awards->items[j], language, resources, &key))
                continue;
            ++count;
            report(L"award ", key.text);
            if (cleanup_mode == apply) {
                if (j < *visible)
                    --*visible;
                delete_entry(awards, j);
            }
        }
    }
    return count;
}

static void DELPHI __attribute__((used, noinline)) cleanup_saved_world(Byte *galaxy, const void *buffer)
{
    if (!buffer || buffer != *(void **)0x81cc70)
        return;
    List *stars = list_at(galaxy, 0x2c);
    if (!valid_list(stars))
        return;
    void *config = *(void **)0x81cfd8;
    void *language = *(void **)0x81cfc0;
    void *resources = *(void **)0x81cfe4;
    unsigned int missiles = 0, awards = 0;
    __atomic_store_n(&report_count, 0, __ATOMIC_RELAXED);
    for (int i = 0; i < stars->count; ++i) {
        Byte *star = stars->items[i];
        if (!star)
            continue;
        missiles += cleanup_missiles(star, config);
        awards += cleanup_awards(star, language, resources);
    }
    report_total(L"missiles = ", missiles);
    report_total(L"awards = ", awards);
}

static int missing_image_override(const Wide *path)
{
    static const Wide *const modes[] = {L"GI,", L"GAI,", L"Alpha,", L"Trans,", L"Simple,"};
    void *resources = *(void **)0x81cfe4;
    if (!resources || !path || !*path)
        return 0;
    for (unsigned int i = 0; i < sizeof(modes) / sizeof(*modes); ++i) {
        const int prefix = prefix_length(path, modes[i]);
        if (prefix) {
            path += prefix;
            break;
        }
    }
    Path key;
    while (*path) {
        if (!prefix_length(path, L"Bm."))
            return 0;
        int length = 0;
        while (path[length] && path[length] != '|' && length < path_capacity - 1) {
            if (path[length] == ',' || path[length] == ':')
                return 0;
            key.text[length] = path[length];
            ++length;
        }
        if (path[length] && path[length] != '|')
            return 0;
        key.text[length] = 0;
        key.bytes = (Word32)length * sizeof(Wide);
        if (!file_exists(resources, key.text))
            return 1;
        if (!path[length])
            return 0;
        path += length + 1;
    }
    return 0;
}

static void DELPHI __attribute__((used, noinline)) cleanup_saved_image(ImageOverride *entry,
                                                                     void *buffer, List *list)
{
    if (!buffer || buffer != *(void **)0x81cc70 || !valid_list(list) || list->count < 1 ||
        list->items[list->count - 1] != entry) {
        load_image(entry, buffer);
        return;
    }
    read_wide(buffer, &entry->form);
    read_wide(buffer, &entry->control);
    read_wide(buffer, &entry->image);
    read_wide(buffer, &entry->original);
    if (!missing_image_override(entry->image)) {
        reapply_image(entry);
        return;
    }
    report(L"interface image ", entry->image);
    if (cleanup_mode == apply) {
        // It was never applied. Bypass the destructor's original-value restore.
        Path unused_form;
        append(&unused_form, 0, L"__FHD_FastCore_DiscardedImageOverride");
        assign_wide(&entry->form, unused_form.text);
        delete_entry(list, list->count - 1);
        free_object(entry);
    }
}

static int DELPHI __attribute__((used, noinline)) block_combat_shot(const Byte *weapon)
{
    if (!weapon || weapon[0x0c] != custom_weapon)
        return 0;
    const Byte *info = *(const Byte *const *)(weapon + 0x80);
    if (!info)
        return 0;
    const int shot_type = info[0x35];
    if (shot_type < 5 || shot_type > 7)
        return 0;
    void *config = *(void **)0x81cfd8;
    Path key;
    const Wide *primary = *(const Wide *const *)(info + 0x60);
    if (!missing_config_block(config, L"SE.", primary, &key) &&
        !missing_config_block(config, L"SE.Missile.w",
                       missile_suffix(info, *(const int *)(weapon + 0x54)), &key))
        return 0;
    report_blocked_shot(key.text);
    return 1;
}

void __attribute__((naked)) cleanup_saved_world_entry(void)
{
    __asm__ volatile(
        "mov %esi, %eax\n"
        "mov %edi, %edx\n"
        "call cleanup_saved_world\n"
        "movl $18, -0x68(%ebp)\n"
        "jmp *resolve_stars_resume\n"
    );
}

void __attribute__((naked)) cleanup_saved_image_entry(void)
{
    __asm__ volatile(
        "mov 0x1c0(%esi), %ecx\n"
        "jmp cleanup_saved_image\n"
    );
}

void __attribute__((naked)) cleanup_combat_shot_entry(void)
{
    __asm__ volatile(
        "mov 0x10(%esi), %eax\n"
        "call block_combat_shot\n"
        "test %eax, %eax\n"
        "jnz 1f\n"
        "mov 0x8(%esi), %eax\n"
        "mov $3, %dl\n"
        "jmp *combat_resume\n"
        "1: jmp *combat_continue\n"
    );
}
