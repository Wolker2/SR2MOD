#include "../scene_recovery.h"

typedef struct {
    Byte prefix[0x10];
    Byte type, owner;
    Byte before_key[0x442];
    const Wide *saved_key;
} TestShip;

typedef struct {
    Byte prefix[0x10];
    int kind;
    const Wide *name, *value;
} Entry;

extern void fail_test(int code) __attribute__((noreturn));
extern const Wide *run_saved_ship_factory(TestShip *, void *, const Wide *, const int *, int);
extern void finish_saved_ship_factory(void);

static const Wide missing_station[] = L"Ruins.RedAxesHQ";
static const Wide missing_ship[] = L"Ship.Custom.Test";
static int buffer, origin[2], asset_checks, config_checks, fail_pirate, fail_all;
static int target_is_ruins;
static char config, resources, block_rc, block_pb, block_sb, block_transport, block_ranger, block_anim;
static TestShip ship;

static int equal(const Wide *a, const Wide *b)
{
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return *a == *b;
}

static void verify_bstr(const Wide *text)
{
    unsigned int length = 0;
    while (text[length])
        ++length;
    if (((const Word32 *)text)[-1] != length * 2)
        fail_test(103);
}

static void *DELPHI find_block(void *root, const Wide *path)
{
    verify_bstr(path);
    ++config_checks;
    if (root == &block_transport || root == &block_ranger) {
        if (equal(path, L"Anim") || equal(path, L"AnimS"))
            return &block_anim;
        fail_test(104);
    }
    if (root != &config)
        fail_test(105);
    if (equal(path, L"SE.Ruins.RC"))
        return &block_rc;
    if (equal(path, L"SE.Ruins.PB"))
        return &block_pb;
    if (equal(path, L"SE.Ruins.SB"))
        return &block_sb;
    if (equal(path, L"SE.Ship.People.Transport"))
        return &block_transport;
    if (equal(path, L"SE.Ship.People.Ranger"))
        return &block_ranger;
    if (!equal(path, L"SE.Ruins.RedAxesHQ") && !equal(path, L"SE.Ship.Custom.Test"))
        fail_test(106);
    return 0;
}

static Entry *DELPHI find_entry(void *block, const Wide *name, Byte raises)
{
    static Entry entry = {.kind = 1};
    verify_bstr(name);
    if (raises)
        fail_test(107);
    if (equal(name, L"Image")) {
        if (block == &block_pb)
            entry.value = L"Bm.Ruins.PB";
        else if (block == &block_transport)
            entry.value = L"Bm.Ship.PeopleT";
        else
            entry.value = L"Bm.Ruins.Test";
    } else if (equal(name, L"ImageMap")) {
        entry.value = L"GI,Bm.Map";
    } else if (equal(name, L"ImageI")) {
        entry.value = L"GAI,Bm.Static?option";
    } else if (equal(name, L"ImageS")) {
        entry.value = L"Bm.Ship.Reduced";
    } else {
        fail_test(108);
    }
    return &entry;
}

static Byte DELPHI asset(void *root, const Wide *path)
{
    verify_bstr(path);
    if (root != &resources)
        fail_test(109);
    ++asset_checks;
    if (equal(path, L"Bm.Ruins.PB"))
        return !fail_pirate && !fail_all;
    if (!equal(path, L"Bm.Map") && !equal(path, L"Bm.Static") &&
        !equal(path, L"Bm.Ruins.Test") && !equal(path, L"Bm.Ship.PeopleT") &&
        !equal(path, L"Bm.Ship.Reduced"))
        fail_test(110);
    return !fail_all;
}

static const Wide *DELPHI factory(const Wide *type, const Wide *key, const int *point)
{
    if (point != origin || !equal(type, target_is_ruins ? L"Ruins" : L"Ship2"))
        fail_test(111);
    return key;
}

static void jump(Word32 address, Word32 target)
{
    volatile Byte *code = (volatile Byte *)address;
    const Word32 rel = target - address - 5;
    code[0] = 0xe9;
    for (unsigned int i = 0; i < 4; ++i)
        code[i + 1] = (Byte)(rel >> (i * 8));
}

static const Wide *run(const Wide *key, void *current, int station, Byte type, Byte owner)
{
    target_is_ruins = station;
    ship.type = type;
    ship.owner = owner;
    ship.saved_key = key;
    const Wide *result = run_saved_ship_factory(&ship, current, key, origin, station);
    if (ship.type != type || ship.owner != owner || ship.saved_key != key)
        fail_test(112);
    return result;
}

void test_ship_scene_recovery(void)
{
    *(void **)0x81cc70u = &buffer;
    *(void **)0x81cfd8u = &config;
    *(void **)0x81cfe4u = &resources;
    jump(0x7b2eb8, (Word32)find_block);
    jump(0x7b1f08, (Word32)find_entry);
    jump(0x4d1c28, (Word32)asset);
    jump(0x782914, (Word32)factory);
    jump(0x6c440a, (Word32)finish_saved_ship_factory);
    jump(0x6c4493, (Word32)finish_saved_ship_factory);

    const Wide installed[] = L"Ruins.PB,122";
    if (run(installed, &buffer, 1, 7, 7) != installed || asset_checks || config_checks != 1)
        fail_test(113);
    if (run(missing_station, 0, 1, 13, 7) != missing_station || asset_checks || config_checks != 1)
        fail_test(114);
    if (!equal(run(missing_station, &buffer, 1, 13, 7), L"Ruins.PB") ||
        !equal(run(missing_station, &buffer, 1, 9, 1), L"Ruins.SB"))
        fail_test(115);
    fail_pirate = 1;
    if (!equal(run(missing_station, &buffer, 1, 13, 7), L"Ruins.RC"))
        fail_test(116);
    if (!equal(run(missing_ship, &buffer, 0, 2, 1), L"Ship.People.Transport"))
        fail_test(117);
    fail_all = 1;
    if (run(missing_station, &buffer, 1, 13, 7) != missing_station ||
        run(missing_ship, &buffer, 0, 2, 1) != missing_ship ||
        run(missing_ship, &buffer, 0, 0, 5) != missing_ship)
        fail_test(118);
}
