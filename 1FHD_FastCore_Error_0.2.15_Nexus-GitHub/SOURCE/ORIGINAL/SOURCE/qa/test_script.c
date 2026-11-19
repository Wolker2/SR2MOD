#include "../script_recovery.h"

typedef struct {
    void *vmt;
    Byte **items;
    int count, capacity;
} List;

typedef struct {
    void *vmt;
    int size, capacity, position;
    Byte *data;
} Buffer;

typedef struct {
    Byte before_filename[0x0c];
    const Wide *filename;
    Byte before_index[8];
    int index;
} Template;

typedef struct {
    Byte before_scripts[0x150];
    List *scripts;
} TestGalaxy;

typedef struct {
    Byte before_binding[0x3cc];
    void *binding;
    Byte before_order[0x7c];
    Byte locked;
    Byte before_player_bindings[0x13f];
    List *player_bindings;
} Ship;

typedef struct {
    Byte prefix[0x10];
    int kind;
    Byte before_read[0x2c];
    Byte was_read;
} Message;

extern void fail_test(int code) __attribute__((noreturn));
extern void run_script_loop(TestGalaxy *, Buffer *, int);
extern void finish_script_loop(void);
extern unsigned short run_script_prepare(Buffer *);
extern void run_script_finish(TestGalaxy *, Buffer *);
extern void finish_script_prepare(void);
extern void finish_script_cleanup(void);

static const Wide autopilot[] = L"Script.Mod_FHD_Autopilot_Controller";
static const Wide another[] = L"Script.Сторонний_мод";
static const Wide available[] = L"Script.OtherAuthor_Installed";
static Byte arena[2048];
static Word32 script_object[0x60 / sizeof(Word32)];
static Byte *script_items[3];
static List scripts = {0, script_items, 0, 3};
static Template kept = {.filename = available, .index = 1};
static Template removed = {.filename = L"Script.Replacement", .index = 0};
static Template removed_later = {.filename = L"Script.OtherReplacement", .index = 2};
static Byte *template_items[] = {(Byte *)&kept, (Byte *)&removed, (Byte *)&removed_later};
static List templates = {0, template_items, 3, 3};
static Ship player, npc, active_npc;
static List player_bindings = {.items = script_items, .count = 1, .capacity = 3};
static TestGalaxy galaxy = {.scripts = &scripts};
static Message orphan = {.kind = 3, .was_read = 1};
static Message shared = {.kind = 3, .was_read = 1};
static Message resolved = {.kind = 4, .was_read = 1};
static char marker_list;
static Buffer input = {.data = arena, .capacity = sizeof(arena)};
static int position, mock_creates, mock_loads, available_start, available_end;
static int save_version = 167;

static int equal(const Wide *a, const Wide *b)
{
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return *a == *b;
}

static void DELPHI *lookup(void *root, const Wide *key)
{
    if (root != (void *)0x410000)
        fail_test(80);
    unsigned int length = 0;
    while (key[length])
        ++length;
    if (length * 2 != ((const Word32 *)key)[-1])
        fail_test(81);
    return equal(key, available) ? root : 0;
}

static Byte *DELPHI create(void *klass, Byte allocate)
{
    if (klass != *(void **)0x645de4 || allocate != 1)
        fail_test(82);
    ++mock_creates;
    return (Byte *)script_object;
}

static int DELPHI add(List *list, Byte *item)
{
    if (list != &scripts || item != (Byte *)script_object || list->count >= list->capacity)
        fail_test(83);
    const int index = list->count++;
    list->items[index] = item;
    return index;
}

static void DELPHI load(Byte *item, Buffer *buffer, TestGalaxy *owner)
{
    if (item != (Byte *)script_object || buffer != &input || owner != &galaxy)
        fail_test(84);
    ++mock_loads;
    if (available_end) {
        if (buffer->position != available_start)
            fail_test(85);
        buffer->position = available_end;
    }
}

static unsigned short DELPHI read_count(Buffer *buffer)
{
    if (buffer != &input)
        fail_test(150);
    return 17; // The fixture supplies the separate native loop counter.
}

static Ship *DELPHI get_player(void)
{
    return &player;
}

static Ship *DELPHI find_ship(const TestGalaxy *owner, Word32 id, Byte raise_if_missing)
{
    if (owner != &galaxy || raise_if_missing)
        fail_test(151);
    if (id == 47)
        return &player;
    if (id == 3)
        return &npc;
    if (id == 4)
        return &active_npc;
    return 0;
}

static Message *DELPHI find_message(const Wide *key, Byte skip_lock)
{
    if (skip_lock)
        fail_test(152);
    unsigned int length = 0;
    while (key[length])
        ++length;
    if (((const Word32 *)key)[-1] != length * 2)
        fail_test(153);
    if (equal(key, L"orphan"))
        return &orphan;
    if (equal(key, L"shared"))
        return &shared;
    if (equal(key, L"resolved"))
        return &resolved;
    return 0;
}

static int DELPHI text_index(void *list, const Wide *key)
{
    if (list != &marker_list)
        fail_test(154);
    return equal(key, L"shared") ? 0 : -1;
}

static void branch(Word32 address, Word32 target)
{
    volatile Byte *at = (volatile Byte *)address;
    const Word32 relative = target - address - 5;
    at[0] = 0xe9;
    for (unsigned int i = 0; i < 4; ++i)
        at[i + 1] = (Byte)(relative >> (i * 8));
}

static char fem_turn_code, fem_process, other_turn_code;
static unsigned int turn_runs;

static void DELPHI mock_run_turn(const void *code, void *process)
{
    if ((code != &fem_turn_code && code != &other_turn_code) || process != &fem_process)
        fail_test(170);
    ++turn_runs;
}

void test_fem_rangers_guard(void)
{
    typedef void (DELPHI *RunTurn)(const void *, void *);
    const Word32 turn_call = 0x64a143;
    const volatile Byte *call = (const volatile Byte *)turn_call;
    Word32 relative = 0;
    for (unsigned int i = 0; i < 4; ++i)
        relative |= (Word32)call[i + 1] << (i * 8);
    RunTurn turn = (RunTurn)(turn_call + 5 + relative);

    struct {
        Byte before_filename[0x08];
        const Wide *filename;
        Byte before_turn[0x2c];
        const void *turn;
    } script = {.filename = L"Script.PC_fem_rangers", .turn = &fem_turn_code};
    const Wide *saved = L"FHD_MODS\\First, Evolution\\EvoFemRangers ,FHD_MODS\\Second";
    const Wide *absent = L"FHD_MODS\\First, Evolution\\EvoFemRangersPlus";
    const Wide *installed = L" \tEVOLUTION/EvoFemRangers\t, FHD_MODS\\Second";
    const Wide *volatile *selected = (const Wide *volatile *)0x81cfc8;
    const Wide *volatile *loaded = (const Wide *volatile *)0x81cfd0;
    *(void **)0x81b848 = &script;
    *selected = absent;
    *loaded = saved;
    branch(0x46cb84, (Word32)mock_run_turn);

    turn(&fem_turn_code, &fem_process);
    if (turn_runs != 0)
        fail_test(171);
    *selected = installed;
    turn(&fem_turn_code, &fem_process);
    if (turn_runs != 1)
        fail_test(172);
    *selected = absent;
    turn(&other_turn_code, &fem_process);
    if (turn_runs != 2)
        fail_test(173);
    script.filename = available;
    turn(&fem_turn_code, &fem_process);
    if (turn_runs != 3)
        fail_test(174);
    script.filename = L"Script.PC_fem_rangers";
    *loaded = absent;
    turn(&fem_turn_code, &fem_process);
    if (turn_runs != 4)
        fail_test(175);
}

static void number(Word32 value, int size)
{
    for (int i = 0; i < size; ++i)
        arena[position++] = (Byte)(value >> (i * 8));
}

static void wide(const Wide *key)
{
    while (*key)
        number(*key++, 2);
    number(0, 2);
}

static void variable(const Wide *key, Byte kind)
{
    wide(key);
    number(kind, 1);
}

static void controller(const Wide *key, Word32 ship, int items, int quest_markers)
{
    wide(key);
    number(1, 4); // Ether
    wide(L"state");
    number(5, 4);
    number(6, 2); // InitCode locals
    variable(L"empty", 0);
    variable(L"integer", 1);
    number(17, 4);
    variable(L"double", 3);
    number(0, 4);
    number(0x3ff00000, 4);
    variable(L"string", 4);
    wide(L"UTF-16 data");
    variable(L"library", 6);
    wide(L"not-a-real-dll");
    variable(L"array", 9);
    number(2, 4); // Generic TVarArrayEC payload
    variable(L"array library", 6); // No payload in a generic array.
    variable(L"subarray", 9);
    number(1, 4);
    variable(L"array value", 2);
    number(0xabcdef01, 4);
    number(1, 2); // TurnCode locals
    variable(L"turn", 2);
    number(17, 4);
    number(1, 4); // Stars
    wide(L"InitStar");
    number(5, 4);
    number(1, 4);
    wide(L"InitPlanet");
    number(42, 4);
    number(0, 4);
    number(items, 4);
    for (int i = 0; i < items; ++i) {
        wide(L"saved item");
        number(1, 1); // CanSell
        number(31, 4);
        number(32, 4);
        number(33, 4);
        wide(L"text 1");
        wide(L"");
        wide(L"text 3");
        wide(L"use");
        wide(L"action");
        number(200, 4);
    }
    number(ship ? 1 : 0, 2);
    if (ship) {
        number(0, 4);
        number(ship, 4);
        for (int i = 0; i < 5; ++i)
            number(i, 4);
        wide(L"PlayerGroup");
        number(0, 1);
        number(1, 1);
    }
    number(quest_markers, 2);
    if (quest_markers > 0)
        wide(L"orphan");
    if (quest_markers > 1)
        wide(L"shared");
    if (quest_markers > 2)
        wide(L"resolved");
}

static void expect_native(int size)
{
    input.position = 0;
    input.size = size;
    scripts.count = 0;
    mock_creates = mock_loads = 0;
    kept.index = 0;
    if (run_script_prepare(&input) != 17)
        fail_test(155);
    run_script_loop(&galaxy, &input, 1);
    if (input.position || scripts.count != 1 || mock_creates != 1 ||
        mock_loads != 1 || kept.index != 0)
        fail_test(86);
}

void test_script_recovery(void)
{
    *(int **)0x813580u = &save_version;
    *(void **)0x81cfe4u = (void *)0x410000;
    *(List **)0x81cc40u = &templates;
    *(Buffer **)0x81cc70u = &input;
    player.locked = npc.locked = active_npc.locked = 3;
    player.player_bindings = &player_bindings;
    active_npc.binding = &marker_list;
    script_object[0x5c / sizeof(Word32)] = (Word32)&marker_list;
    branch(0x4d1934, (Word32)lookup);
    branch(0x649600, (Word32)create);
    branch(0x4161c0, (Word32)add);
    branch(0x64f738, (Word32)load);
    branch(0x7d2c17, (Word32)finish_script_loop);
    branch(0x7b8254, (Word32)read_count);
    branch(0x7d2b8c, (Word32)finish_script_prepare);
    branch(0x7d434c, (Word32)finish_script_cleanup);
    branch(0x747300, (Word32)get_player);
    branch(0x7d7c50, (Word32)find_ship);
    branch(0x7a5a20, (Word32)find_message);
    branch(0x799b88, (Word32)text_index);

    controller(autopilot, 47, 0, 0);
    available_start = position;
    controller(available, 0, 0, 0);
    available_end = position;
    controller(L"Script.Mod_EvoArmada", 3, 1, 3);
    controller(another, 4, 0, 0);
    const int end = position;
    number(0xbeef, 2);
    input.size = position;
    if (run_script_prepare(&input) != 17)
        fail_test(156);
    run_script_loop(&galaxy, &input, 4);
    if (input.position != end || scripts.count != 1 || mock_creates != 1 ||
        mock_loads != 1 || kept.index != 0 || removed.index != -1 || removed_later.index != -1)
        fail_test(87);
    if (npc.locked != 3 || orphan.kind != 3)
        fail_test(157);
    run_script_finish(&galaxy, &input);
    if (npc.locked || player.locked != 3 || active_npc.locked != 3 ||
        orphan.kind != 5 || orphan.was_read ||
        shared.kind != 3 || !shared.was_read || resolved.kind != 4 || !resolved.was_read)
        fail_test(158);

    available_end = 0;
    position = 0;
    controller(autopilot, 47, 1, 3);
    const int script_end = position;
    number(0xbeef, 2);
    for (int size = 1; size < script_end + 2; ++size)
        expect_native(size);
    templates.count = 1;
    kept.filename = L"Script/Mod_FHD_Autopilot_Controller";
    expect_native(position);
    kept.filename = available;

    save_version = 161;
    expect_native(position);
    save_version = 168;
    expect_native(position);
    save_version = 167;
    *(Buffer **)0x81cc70u = 0;
    expect_native(position);
    *(Buffer **)0x81cc70u = &input;

    // A retry resets pending work even if the previous load stopped early.
    position = 0;
    controller(autopilot, 3, 1, 1);
    number(0xbeef, 2);
    input.position = 0;
    input.size = position;
    scripts.count = 0;
    npc.locked = 2;
    orphan.kind = 3;
    if (run_script_prepare(&input) != 17)
        fail_test(159);
    run_script_loop(&galaxy, &input, 1);
    if (input.position != position - 2 || scripts.count)
        fail_test(160);
    if (run_script_prepare(&input) != 17)
        fail_test(161);
    run_script_finish(&galaxy, &input);
    if (npc.locked != 2 || orphan.kind != 3)
        fail_test(162);
}
