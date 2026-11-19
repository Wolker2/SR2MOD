#include "../scene_recovery.h"

typedef struct {
    const void *vmt;
    Byte prefix[0x14];
    const Wide *key;
} TestScene;

typedef struct {
    Byte prefix[0x10];
    int kind;
    const Wide *name, *value;
} Entry;

extern void fail_test(int code) __attribute__((noreturn));
extern void *run_scene_config(TestScene *, void *, const Wide *);
extern void finish_scene_config(void);

typedef struct {
    Word32 class_reference;
    const Wide *original, *replacement;
    char block;
} TestCase;

static TestCase batch[] = {
    {0x65b880, L"SE.Ship.Akrin.InhibitorOn", L"Ship.Akrin.A", 0},
    {0x65b880, L"SE.Ship.Blazer.NoLongerInstalled", L"Ship.Blazer.B1", 0},
    {0x65b880, L"SE.Ship.Terron.Unknown", L"Ship.Terron.T1", 0},
    {0x65b880, L"SE.Ship.Keller.Unknown", L"Ship.Keller.K1", 0},
    {0x65b880, L"SE.Ship.AkrinFemale.Unknown", L"Ship.AkrinFemale.J", 0},
    {0x65b880, L"SE.Ship.OtherAuthor.Unknown", L"Ship.People.Ranger", 0},
    {0x6991b0, L"SE.Ruins.Unknown", L"Ruins.RC", 0},
    {0x69181c, L"SE.Ruins.Unknown", L"Ruins.RG", 0},
    {0x6c1f54, L"SE.Star.Unknown", L"Star.00", 0},
    {0x7276d0, L"SE.Hole.Unknown", L"Hole.00", 0},
    {0x695cf4, L"SE.Asteroid.Unknown", L"Asteroid.0", 0},
    {0x695550, L"SE.Sputnik.Unknown", L"Sputnik.00", 0}
};
static char config, resources, installed, anim, native_error, buffer;
static int assignments, native_calls, read_calls, missing_asset;
static char missile_w4, missile_w15;
static int stock_missile_image;
static const Wide present[] = L"SE.Ship.Akrin.Existing";

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
        fail_test(120);
}

static void *DELPHI native_get(void *root, const Wide *path)
{
    if (root != &config || !path)
        fail_test(121);
    ++native_calls;
    return &native_error;
}

static void *DELPHI lookup(void *root, const Wide *path)
{
    if (root != &config) {
        if (equal(path, L"Anim") || equal(path, L"AnimS"))
            return &anim;
        fail_test(122);
    }
    if (equal(path, present))
        return &installed;
    if (equal(path, L"SE.Missile.w4") || equal(path, L"SE.Missile.w15")) {
        verify_bstr(path);
        return equal(path, L"SE.Missile.w4") ? &missile_w4 : &missile_w15;
    }
    if (equal(path, L"SE.Missile.wexisting"))
        return &installed;
    if (path[0] != 'S' || path[1] != 'E' || path[2] != '.')
        fail_test(123);
    for (unsigned int i = 0; i < sizeof(batch) / sizeof(*batch); ++i)
        if (equal(path + 3, batch[i].replacement)) {
            verify_bstr(path);
            return &batch[i].block;
        }
    return 0;
}

static Entry *DELPHI entry(void *block, const Wide *name, Byte raises)
{
    static Entry value = {.kind = 1};
    if (!block || raises)
        fail_test(124);
    verify_bstr(name);
    if (equal(name, L"ImageMap"))
        value.value = L"Alpha,Mock.Minimap";
    else if (equal(name, L"ImageI"))
        value.value = L"GI,Mock.Static";
    else if (equal(name, L"ImageS"))
        value.value = L"Mock.Reduced";
    else if (equal(name, L"Anim"))
        value.value = L"GAI,Mock.Animation";
    else if (equal(name, L"Image"))
        value.value = L"Mock.Image";
    else
        fail_test(125);
    return &value;
}

static Byte DELPHI readable(void *root, const Wide *path)
{
    if (root != &resources)
        fail_test(126);
    verify_bstr(path);
    ++read_calls;
    if (equal(path, L"Bm.Missile.w4"))
        return stock_missile_image == 4;
    if (equal(path, L"Bm.Missile.w15"))
        return stock_missile_image == 15;
    if (path[0] != 'M' || path[4] != '.')
        fail_test(126);
    return !missing_asset;
}

static void DELPHI assign(const Wide **destination, const Wide *key)
{
    verify_bstr(key);
    ++assignments;
    *destination = key;
}

static void jump(Word32 address, Word32 target)
{
    volatile Byte *code = (volatile Byte *)address;
    const Word32 rel = target - address - 5;
    code[0] = 0xe9;
    for (unsigned int i = 0; i < 4; ++i)
        code[i + 1] = (Byte)(rel >> (i * 8));
}

void test_scene_batch_recovery(void)
{
    *(void **)0x81cc70u = &buffer;
    *(void **)0x81cfd8u = &config;
    *(void **)0x81cfe4u = &resources;
    jump(0x7b2dec, (Word32)native_get);
    jump(0x7b2eb8, (Word32)lookup);
    jump(0x7b1f08, (Word32)entry);
    jump(0x4d1c28, (Word32)readable);
    jump(0x405d54, (Word32)assign);
    jump(0x786602, (Word32)finish_scene_config);

    for (unsigned int i = 0; i < sizeof(batch) / sizeof(*batch); ++i) {
        TestCase *item = &batch[i];
        TestScene scene = {.vmt = *(void **)item->class_reference, .key = item->original + 3};
        if (run_scene_config(&scene, &config, item->original) != &item->block ||
            !equal(scene.key, item->replacement))
            fail_test(127 + i);
    }
    if (assignments != (int)(sizeof(batch) / sizeof(*batch)) || native_calls)
        fail_test(139);
    TestScene scene = {.vmt = *(void **)0x65b880u, .key = present + 3};
    const int previous_reads = read_calls;
    if (run_scene_config(&scene, &config, present) != &installed ||
        scene.key != present + 3 || read_calls != previous_reads)
        fail_test(140);
    scene.vmt = *(void **)0x695550u;
    if (run_scene_config(&scene, &config, batch[0].original) != &native_error ||
        scene.key != present + 3 || read_calls != previous_reads)
        fail_test(141);
    scene.vmt = *(void **)0x65b880u;
    missing_asset = 1;
    if (run_scene_config(&scene, &config, batch[0].original) != &native_error ||
        scene.key != present + 3)
        fail_test(142);
    const int failed_reads = read_calls;
    *(void **)0x81cc70u = 0;
    if (run_scene_config(&scene, &config, batch[0].original) != &native_error ||
        scene.key != present + 3 || read_calls != failed_reads)
        fail_test(143);

    static const Wide unknown_missile[] = L"SE.Missile.wtornado";
    scene.vmt = *(void **)0x5ccbfcu;
    scene.key = unknown_missile + 3;
    stock_missile_image = 4;
    if (run_scene_config(&scene, &config, unknown_missile) != &missile_w4 ||
        !equal(scene.key, L"Missile.w4"))
        fail_test(251);
    stock_missile_image = 15;
    scene.key = unknown_missile + 3;
    if (run_scene_config(&scene, &config, unknown_missile) != &missile_w15 ||
        !equal(scene.key, L"Missile.w15"))
        fail_test(252);
    const int missile_reads = read_calls;
    if (run_scene_config(&scene, &config, L"SE.Missile.wexisting") != &installed ||
        read_calls != missile_reads)
        fail_test(253);
    stock_missile_image = 0;
    if (run_scene_config(&scene, &config, unknown_missile) != &native_error)
        fail_test(254);
}
