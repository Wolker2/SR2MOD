#include "../scene_recovery.h"

typedef struct {
    Byte data[0x140];
    int radius;
    void *graphic;
    const Wide *saved_key;
} TestPlanet;

typedef struct {
    Byte prefix[0x18];
    const Wide *key;
    Byte before_texture[0x30];
    const Wide *texture;
    Byte before_minimap[0x18];
    const Wide *minimap;
    Byte before_cloud[0x18];
    const Wide *cloud;
    Byte before_background[0x48];
    const Wide *background;
    Byte enables_quests;
} TestVisual;

typedef struct {
    int style, variant, radius;
    const TestVisual *visual;
} Template;

extern void fail_test(int code) __attribute__((noreturn));
extern const Wide *run_saved_planet_factory(TestPlanet *, void *, const Wide *, const int *);
extern void finish_saved_planet_factory(void);

static const Wide *missing = L"Planet.208";
static const Wide *configured = L"Planet.001,legacy";
static TestVisual visual_near = {.key = L"Planet.122", .texture = L"Texture.90",
                                 .minimap = L"Minimap", .background = L"214"};
static TestVisual visual_exact = {.key = L"Planet.151", .texture = L"Texture.100",
                                  .minimap = L"Minimap", .background = L"114",
                                  .cloud = L"Cloud?RGBA"};
static TestVisual visual_no_cloud = {.key = L"Planet.152", .texture = L"Texture.100",
                                     .minimap = L"Minimap", .background = L"114",
                                     .cloud = L"MissingCloud"};
static TestVisual visual_quest = {.key = L"Planet.150", .texture = L"Texture.100",
                                  .minimap = L"Minimap", .background = L"114",
                                  .enables_quests = 1};
static struct {
    int refcount, count;
    Template items[5];
} templates = {1, 5, {{0, 0, 90, &visual_near}, {0, 0, 100, &visual_quest},
                      {0, 0, 100, &visual_no_cloud}, {1, 0, 100, &visual_exact},
                      {0, 0, 100, &visual_exact}}};
static int point[2], load_buffer, config_lookups, asset_lookups, missing_assets;
static TestPlanet planet;

static int equal(const Wide *left, const Wide *right)
{
    while (*left && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static void validate_bstr(const Wide *path)
{
    unsigned int length = 0;
    while (path[length])
        ++length;
    if (((const Word32 *)path)[-1] != length * 2)
        fail_test(89);
}

static void *DELPHI lookup(void *config, const Wide *path)
{
    ++config_lookups;
    if (config != (void *)0x411000)
        fail_test(90);
    validate_bstr(path);
    if (equal(path, L"SE.Planet.001"))
        return config;
    if (!equal(path, L"SE.Planet.208"))
        fail_test(91);
    return 0;
}

static Byte DELPHI available(void *root, const Wide *path)
{
    ++asset_lookups;
    if (root != (void *)0x410000)
        fail_test(92);
    validate_bstr(path);
    if (!equal(path, L"Texture.90") && !equal(path, L"Texture.100") &&
        !equal(path, L"Minimap") && !equal(path, L"Cloud") &&
        !equal(path, L"MissingCloud") && !equal(path, L"Bm.PlanetBG.214") &&
        !equal(path, L"Bm.PlanetBG.114"))
        fail_test(100);
    return !missing_assets && !equal(path, L"MissingCloud");
}

static const Wide *DELPHI factory(const Wide *name, const Wide *key, const int *origin)
{
    if (!equal(name, L"Planet") || origin != point)
        fail_test(93);
    return key;
}

static void branch(Word32 address, Word32 target)
{
    volatile Byte *code = (volatile Byte *)address;
    const Word32 relative = target - address - 5;
    code[0] = 0xe9;
    for (unsigned int i = 0; i < 4; ++i)
        code[i + 1] = (Byte)(relative >> (i * 8));
}

static const Wide *try_factory(const Wide *key, void *context, int radius)
{
    planet.radius = radius;
    planet.saved_key = key;
    const Wide *result = run_saved_planet_factory(&planet, context, key, point);
    if (planet.saved_key != key || planet.radius != radius || planet.graphic)
        fail_test(94);
    return result;
}

void test_planet_recovery(void)
{
    *(void **)0x81cc70u = &load_buffer;
    *(void **)0x81cfd8u = (void *)0x411000;
    *(void **)0x81cfe4u = (void *)0x410000;
    *(Template **)0x81cea4u = templates.items;
    branch(0x7b2eb8, (Word32)lookup);
    branch(0x4d1c28, (Word32)available);
    branch(0x782914, (Word32)factory);
    branch(0x72e1a3, (Word32)finish_saved_planet_factory);
    if (try_factory(configured, &load_buffer, 100) != configured ||
        asset_lookups || config_lookups != 1)
        fail_test(95);
    if (try_factory(missing, 0, 100) != missing || asset_lookups || config_lookups != 1)
        fail_test(96);
    if (try_factory(missing, &load_buffer, 100) != visual_exact.key ||
        config_lookups != 2)
        fail_test(97);
    if (try_factory(missing, &load_buffer, 91) != visual_near.key)
        fail_test(98);
    missing_assets = 1;
    if (try_factory(missing, &load_buffer, 100) != missing)
        fail_test(99);
}
