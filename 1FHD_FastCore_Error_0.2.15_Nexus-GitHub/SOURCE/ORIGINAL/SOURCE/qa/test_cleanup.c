#include "../orphan_cleanup.h"

typedef struct {
    Word32 bytes;
    Wide text[160];
} Key;

typedef struct {
    void *vmt;
    void **items;
    int count, capacity;
} List;

typedef struct {
    void *vmt;
    const Wide *form, *control, *image, *original;
} Image;

typedef struct {
    Byte prefix[0x10];
    int kind;
    const Wide *name, *value;
} Entry;

extern void fail_test(int) __attribute__((noreturn));
extern int run_cleanup_world(void *, void *);
extern void run_cleanup_image(void *, Image *, void *);
extern int run_cleanup_shot(void *);
extern void finish_cleanup_world(void);
extern void finish_cleanup_shot(void);
extern void skip_cleanup_shot(void);

static char config, language, resources, file_buffer, other_buffer;
static Byte galaxy[0x1c4], star[2][0x40], ship[2][0x3f0];
static Byte missile[4][0x78], weapon[0x84], good_info[0x78], bad_info[0x78];
static Word32 combat_event[5];
static Image image;
static Key bad_name, good_name, primary, fields[4], dead_form;
static const Wide *file_fields[4];
static int read_index, deleted, freed, reapplied, native_load, checks;

static int equals(const Wide *left, const Wide *right)
{
    if (!left || !right)
        return left == right;
    while (*left && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static const Wide *make_key(Key *key, const Wide *text)
{
    unsigned int length = 0;
    while (text[length]) {
        key->text[length] = text[length];
        ++length;
    }
    key->text[length] = 0;
    key->bytes = length * 2;
    return key->text;
}

static void verify_key(const Wide *text)
{
    unsigned int length = 0;
    while (text[length])
        ++length;
    if (((const Word32 *)text)[-1] != length * 2)
        fail_test(215);
}

static void branch(Word32 site, Word32 target)
{
    volatile Byte *code = (volatile Byte *)site;
    Word32 relative = target - site - 5;
    code[0] = 0xe9;
    for (int i = 0; i < 4; ++i)
        code[i + 1] = (Byte)(relative >> (i * 8));
}

static void *DELPHI mock_block(void *root, const Wide *path)
{
    if (root != &config)
        fail_test(216);
    verify_key(path);
    ++checks;
    return equals(path, L"SE.Missile.wactive") || equals(path, L"SE.Weapon.Active") ? root : 0;
}

static Entry *DELPHI mock_entry(void *root, const Wide *path, Byte raises)
{
    if (root != &language || raises)
        fail_test(217);
    verify_key(path);
    static Entry entry = {.kind = 1};
    return equals(path, L"Reward.49.Name") ? &entry : 0;
}

static Byte DELPHI mock_file(void *root, const Wide *path)
{
    if (root != &resources)
        fail_test(218);
    verify_key(path);
    return equals(path, L"Bm.FormRewards.2_58") || equals(path, L"Bm.Form.Original");
}

static void DELPHI mock_delete(List *list, int index)
{
    if (index < 0 || index >= list->count)
        fail_test(219);
    --list->count;
    for (int i = index; i < list->count; ++i)
        list->items[i] = list->items[i + 1];
    ++deleted;
}

static void DELPHI mock_free(void *object)
{
    if (object == &image) {
        if (!equals(image.form, L"__FHD_FastCore_DiscardedImageOverride"))
            fail_test(220);
    } else if ((object != missile[0] && object != missile[3]) ||
               *(void **)((Byte *)object + 0x38)) {
        fail_test(221);
    }
    ++freed;
}

static void DELPHI mock_read(void *buffer, const Wide **out)
{
    if (buffer != &file_buffer || read_index >= 4)
        fail_test(222);
    *out = make_key(&fields[read_index], file_fields[read_index]);
    ++read_index;
}

static void DELPHI mock_assign(const Wide **out, const Wide *text)
{
    verify_key(text);
    *out = make_key(&dead_form, text);
}

static void DELPHI mock_reapply(Image *entry)
{
    if (entry != &image)
        fail_test(223);
    ++reapplied;
}

static void DELPHI mock_native_load(Image *entry, void *buffer)
{
    if (entry != &image || buffer != &other_buffer)
        fail_test(224);
    ++native_load;
}

void test_orphan_cleanup(int apply)
{
    branch(0x7d2e0e, (Word32)finish_cleanup_world);
    *(Byte *)0x7d3874u = 0xc3;
    branch(0x7b2eb8, (Word32)mock_block);
    branch(0x7b1f08, (Word32)mock_entry);
    branch(0x4d1c28, (Word32)mock_file);
    branch(0x41620c, (Word32)mock_delete);
    branch(0x4045ac, (Word32)mock_free);
    branch(0x405d54, (Word32)mock_assign);
    branch(0x7b8750, (Word32)mock_read);
    branch(0x800250, (Word32)mock_native_load);
    branch(0x8002f0, (Word32)mock_reapply);
    *(void **)0x81cc70u = &file_buffer;
    *(void **)0x81cfd8u = &config;
    *(void **)0x81cfc0u = &language;
    *(void **)0x81cfe4u = &resources;
    *(const Wide **)(bad_info + 4) = make_key(&bad_name, L"tornado");
    *(const Wide **)(good_info + 4) = make_key(&good_name, L"active");
    *(const Wide **)(good_info + 0x60) = make_key(&primary, L"Weapon.Active");
    for (int i = 0; i < 4; ++i) {
        *(void **)missile[i] = i == 2 ? 0 : *(void **)0x5cd678u;
        *(void **)(missile[i] + 0x38) = (void *)42u;
        *(void **)(missile[i] + 0x74) = i == 1 ? good_info : bad_info;
    }
    void *missiles0[] = {missile[0], missile[1], missile[2]};
    void *missiles1[] = {missile[3]};
    void *awards0[] = {(void *)68, (void *)49, (void *)3, (void *)77, (void *)58};
    void *awards1[] = {(void *)80};
    List m0 = {0, missiles0, 3, 3}, m1 = {0, missiles1, 1, 1};
    List a0 = {0, awards0, 5, 5}, a1 = {0, awards1, 1, 1};
    void *ships0[] = {ship[0]}, *ships1[] = {ship[1]};
    List s0 = {0, ships0, 1, 1}, s1 = {0, ships1, 1, 1};
    void *stars[] = {star[0], star[1]};
    List world = {0, stars, 2, 2};
    *(List **)(galaxy + 0x2c) = &world;
    *(List **)(star[0] + 0x38) = &m0;
    *(List **)(star[1] + 0x38) = &m1;
    *(List **)(star[0] + 0x2c) = &s0;
    *(List **)(star[1] + 0x2c) = &s1;
    *(List **)(ship[0] + 0x3e8) = &a0;
    *(List **)(ship[1] + 0x3e8) = &a1;
    *(int *)(ship[0] + 0x3ec) = 3;
    *(int *)(ship[1] + 0x3ec) = 1;
    if (run_cleanup_world(galaxy, &other_buffer) != 18 || deleted || checks)
        fail_test(225);
    if (run_cleanup_world(galaxy, &file_buffer) != 18 || checks != 3)
        fail_test(226);
    if (apply) {
        if (deleted != 5 || freed != 2 || m0.count != 2 || m1.count ||
            missiles0[0] != missile[1] || missiles0[1] != missile[2] ||
            a0.count != 3 || a1.count || awards0[0] != (void *)49 ||
            awards0[1] != (void *)3 || awards0[2] != (void *)58 ||
            *(int *)(ship[0] + 0x3ec) != 2 || *(int *)(ship[1] + 0x3ec))
            fail_test(227);
        run_cleanup_world(galaxy, &file_buffer);
        if (deleted != 5 || freed != 2)
            fail_test(228);
    } else if (deleted || freed || m0.count != 3 || a0.count != 5 ||
               *(int *)(ship[0] + 0x3ec) != 3) {
        fail_test(229);
    }

    void *images[] = {&image};
    List list = {0, images, 1, 1};
    *(List **)(galaxy + 0x1c0) = &list;
    file_fields[0] = L"Ship";
    file_fields[1] = L"Frame";
    file_fields[2] = L"GI,Bm.Form.Original";
    file_fields[3] = L"GI,Bm.Form.Original";
    run_cleanup_image(galaxy, &image, &other_buffer);
    run_cleanup_image(galaxy, &image, &file_buffer);
    if (native_load != 1 || read_index != 4 || reapplied != 1 || list.count != 1)
        fail_test(230);
    read_index = 0;
    file_fields[2] = L"Bm.Form.Original|Bm.Form.Disabled";
    run_cleanup_image(galaxy, &image, &file_buffer);
    if (read_index != 4 || reapplied != 1 || list.count != !apply || freed != apply * 3)
        fail_test(231);

    if (apply) {
        branch(0x7f98d9, (Word32)finish_cleanup_shot);
        branch(0x7fa075, (Word32)skip_cleanup_shot);
        combat_event[2] = 0x12345678;
        combat_event[4] = (Word32)weapon;
        weapon[0x0c] = 68;
        *(void **)(weapon + 0x80) = bad_info;
        bad_info[0x35] = 6;
        if (run_cleanup_shot(combat_event) != 1)
            fail_test(232);
        *(void **)(weapon + 0x80) = good_info;
        good_info[0x35] = 6;
        if (run_cleanup_shot(combat_event) != 0)
            fail_test(233);
        weapon[0x0c] = 51;
        if (run_cleanup_shot(combat_event) != 0)
            fail_test(234);
    }
}

extern int run_countable_size(void *, void *, const Wide *);
extern int run_countable_merge(void *, void *);
extern void finish_countable_missing(void);
extern void finish_countable_found(void);
extern void finish_countable_merge(void);

static Byte loose_item[0x68], incoming_stack[0x68], pickup_frame[0x1c];
static Key missing_stack, current_stack, full_stack_path;
static int native_pickups, cleared_references, deleted_stacks, film_commands, film_token, film_object;

static void *DELPHI mock_countable_block(void *root, const Wide *path)
{
    if (root != &language)
        fail_test(236);
    verify_key(path);
    return equals(path, L"Items.CustomCountables.ActiveStack") ? root : 0;
}

static int DELPHI mock_native_unit_size(const Byte *item)
{
    if (item != loose_item)
        fail_test(237);
    return 7;
}

static int DELPHI mock_index(List *list, const void *item)
{
    for (int i = 0; i < list->count; ++i)
        if (list->items[i] == item)
            return i;
    return -1;
}

static void DELPHI mock_clear_references(void *owner, const void *item)
{
    if (owner != star[0] || item != loose_item || film_commands != 2)
        fail_test(238);
    ++cleared_references;
}

static void DELPHI mock_stack_free(void *item)
{
    if (item != loose_item || cleared_references != 1)
        fail_test(239);
    ++deleted_stacks;
}

static void DELPHI mock_film_detach(void *film, int step, void *object)
{
    if (film != &film_token || step != 17 || object != &film_object || film_commands)
        fail_test(240);
    ++film_commands;
}

static void DELPHI mock_film_release(void *film, int step, void *object)
{
    if (film != &film_token || step != 17 || object != &film_object || film_commands != 1)
        fail_test(241);
    ++film_commands;
}

static void mock_native_pickup(const Byte *frame)
{
    if (frame != pickup_frame + sizeof(pickup_frame))
        fail_test(242);
    ++native_pickups;
}

static void (*pickup_target(Word32 address))(const Byte *)
{
    const volatile Byte *p = (const volatile Byte *)address;
    Word32 offset = 0;
    for (int i = 0; i < 4; ++i)
        offset |= (Word32)p[i + 1] << (i * 8);
    return (void (*)(const Byte *))(address + 5 + offset);
}

void test_countable_recovery(int apply)
{
    branch(0x7076c3, (Word32)finish_countable_found);
    branch(0x7076ee, (Word32)finish_countable_missing);
    branch(0x7079d2, (Word32)finish_countable_merge);
    branch(0x7b2eb8, (Word32)mock_countable_block);
    branch(0x707680, (Word32)mock_native_unit_size);
    *(void **)0x81cfc0u = &language;
    const Wide *missing = make_key(&missing_stack, L"Mod_ExpRC_UnknownMM");
    const Wide *registered = make_key(&current_stack, L"ActiveStack");
    loose_item[0x0c] = 75;
    *(const Wide **)(loose_item + 0x38) = missing;
    *(int *)(loose_item + 0x18) = 18;
    *(int *)(loose_item + 0x60) = 6;
    if (run_countable_size(loose_item, &language, make_key(&full_stack_path,
            L"Items.CustomCountables.Mod_ExpRC_UnknownMM")) != 3)
        fail_test(243);
    if (run_countable_size(loose_item, &language, make_key(&full_stack_path,
            L"Items.CustomCountables.ActiveStack")) != 7)
        fail_test(244);
    *(int *)(loose_item + 0x60) = 0;
    if (run_countable_size(loose_item, 0, full_stack_path.text) != 1)
        fail_test(245);
    *(int *)(loose_item + 0x60) = 10;
    *(int *)(incoming_stack + 0x18) = 12;
    if (run_countable_merge(loose_item, incoming_stack) != 30)
        fail_test(246);
    *(const Wide **)(loose_item + 0x38) = registered;
    if (run_countable_merge(loose_item, incoming_stack) != 70)
        fail_test(247);
    if (!apply)
        return;

    void *items[] = {loose_item};
    List list = {0, items, 1, 1};
    *(List **)(star[0] + 0x30) = &list;
    Byte *frame = pickup_frame + sizeof(pickup_frame);
    *(void **)(frame - 4) = loose_item;
    *(void **)(frame - 12) = star[0];
    *(int *)(frame - 0x1c) = 17;
    *(void **)(loose_item + 0x28) = &film_object;
    *(void **)0x81cc60u = &film_token;
    frame[-13] = 1;
    branch(0x4163c4, (Word32)mock_index);
    branch(0x7e145c, (Word32)mock_clear_references);
    branch(0x4045ac, (Word32)mock_stack_free);
    branch(0x783ca4, (Word32)mock_film_detach);
    branch(0x783ce0, (Word32)mock_film_release);
    branch(0x7f4f5c, (Word32)mock_native_pickup);
    void (*pickup)(const Byte *) = pickup_target(0x7fc549);
    if (pickup != pickup_target(0x7fc897))
        fail_test(248);
    pickup(frame);
    *(const Wide **)(loose_item + 0x38) = missing;
    *(void **)(loose_item + 0x2c) = &film_token;
    pickup(frame);
    *(void **)(loose_item + 0x2c) = 0;
    loose_item[0x34] = 1;
    pickup(frame);
    loose_item[0x34] = 0;
    if (native_pickups != 3 || list.count != 1 || cleared_references || deleted_stacks)
        fail_test(249);
    pickup(frame);
    if (list.count || native_pickups != 3 || cleared_references != 1 ||
        deleted_stacks != 1 || film_commands != 2)
        fail_test(250);
}

extern List *run_live_missiles_open(void *);
extern List *run_live_missiles_refresh(void *);
extern void finish_live_missiles_open(void);
extern void finish_live_missiles_refresh(void);

static List live_list;
static int live_removed;
static Byte special_template[4 + 0x124];
static const Byte *template_data = special_template + 4;

static void DELPHI mock_live_missile_free(void *object)
{
    Byte *shot = object;
    if (*(void **)(shot + 0x38) != star[0] || *(void **)(shot + 0x5c) ||
        (shot != missile[0] && shot != missile[2]))
        fail_test(256);
    const int index = mock_index(&live_list, object);
    if (index < 0)
        fail_test(257);
    mock_delete(&live_list, index);
    ++live_removed;
}

void test_live_missile_cleanup(void)
{
    branch(0x7e261f, (Word32)finish_live_missiles_open);
    branch(0x7e27c7, (Word32)finish_live_missiles_refresh);
    branch(0x7b2eb8, (Word32)mock_block);
    branch(0x4045ac, (Word32)mock_live_missile_free);
    *(void **)0x81cfd8u = &config;
    *(void **)0x81cc70u = 0;
    *(const Byte ***)0x8136b4u = &template_data;
    *(int *)special_template = 1;
    *(const Wide **)(special_template + 4 + 0x110) = bad_name.text;
    for (int i = 0; i < 4; ++i)
        *(void **)(missile[i] + 0x38) = star[0];
    *(void **)missile[2] = *(void **)0x5cd610u;
    *(int *)(missile[2] + 0x20) = 1;
    *(void **)(missile[3] + 0x5c) = &film_object;

    void *items[] = {missile[0], missile[1], missile[2], missile[3]};
    live_list = (List){0, items, 4, 4};
    *(List **)(star[0] + 0x38) = &live_list;
    if (run_live_missiles_refresh(star[0]) != &live_list || live_removed != 2 ||
        live_list.count != 2 || items[0] != missile[1] || items[1] != missile[3])
        fail_test(258);
    if (run_live_missiles_open(star[0]) != &live_list || live_removed != 2)
        fail_test(259);
    items[2] = missile[0];
    ++live_list.count;
    if (run_live_missiles_open(star[0]) != &live_list || live_removed != 3 ||
        live_list.count != 2)
        fail_test(260);
}
