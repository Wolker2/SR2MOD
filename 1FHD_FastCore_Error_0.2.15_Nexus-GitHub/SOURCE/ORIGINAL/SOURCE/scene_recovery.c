#include "scene_recovery.h"

enum { path_capacity = 256, station_center = 6,
       station_custom = 13, owner_pirate = 7 };

#define BSTR_LITERAL(name, literal) \
    static const struct { Word32 bytes; Wide text[sizeof(literal) / sizeof(Wide)]; } \
        name = {sizeof(literal) - sizeof(Wide), literal}

BSTR_LITERAL(center, L"Ruins.RC");
BSTR_LITERAL(pirate_base, L"Ruins.PB");
BSTR_LITERAL(military, L"Ruins.WB");
BSTR_LITERAL(science, L"Ruins.SB");
BSTR_LITERAL(business, L"Ruins.BK");
BSTR_LITERAL(medical, L"Ruins.MC");
BSTR_LITERAL(dominion, L"Ruins.CB");
BSTR_LITERAL(ranger, L"Ship.People.Ranger");
BSTR_LITERAL(transport, L"Ship.People.Transport");
BSTR_LITERAL(pirate_ship, L"Ship.People.Pirate");
BSTR_LITERAL(warrior, L"Ship.People.Warrior");
BSTR_LITERAL(tranclucator, L"Ship.Tranclucator");
BSTR_LITERAL(image, L"Image");
BSTR_LITERAL(image_still, L"ImageI");
BSTR_LITERAL(image_small, L"ImageS");
BSTR_LITERAL(image_map, L"ImageMap");
BSTR_LITERAL(anim, L"Anim");
BSTR_LITERAL(anim_small, L"AnimS");
BSTR_LITERAL(akrin, L"Ship.Akrin.A");
BSTR_LITERAL(akrin_female, L"Ship.AkrinFemale.J");
BSTR_LITERAL(keller, L"Ship.Keller.K1");
BSTR_LITERAL(blazer, L"Ship.Blazer.B1");
BSTR_LITERAL(terron, L"Ship.Terron.T1");
BSTR_LITERAL(star_visual, L"Star.00");
BSTR_LITERAL(hole_visual, L"Hole.00");
BSTR_LITERAL(asteroid_visual, L"Asteroid.0");
BSTR_LITERAL(sputnik_visual, L"Sputnik.00");
BSTR_LITERAL(orbital_station, L"Ruins.RG");
BSTR_LITERAL(stock_missile, L"Missile.w4");
BSTR_LITERAL(stock_missile_15, L"Missile.w15");
BSTR_LITERAL(stock_missile_18, L"Missile.w18");

typedef struct {
    Word32 bytes;
    Wide text[path_capacity];
} ConfigPath;

typedef struct {
    Byte before_key[0x18];
    const Wide *key;
    Byte before_texture[0x30];
    const Wide *texture;
    Byte before_minimap[0x18];
    const Wide *minimap;
    Byte before_cloud1[0x18];
    const Wide *cloud1;
    Byte before_cloud2[0x10];
    const Wide *cloud2;
    Byte before_cloud3[0x10];
    const Wide *cloud3;
    Byte before_background[0x20];
    const Wide *background;
    Byte enables_quests;
} PlanetVisual;

typedef struct {
    int style, variant, radius;
    const PlanetVisual *visual;
} PlanetTemplate;

_Static_assert(__builtin_offsetof(PlanetVisual, key) == 0x18, "TObjectSE.GraphKey");
_Static_assert(__builtin_offsetof(PlanetVisual, texture) == 0x4c, "TPlanetSE.ImagePath");
_Static_assert(__builtin_offsetof(PlanetVisual, minimap) == 0x68, "TPlanetSE.MinimapImagePath");
_Static_assert(__builtin_offsetof(PlanetVisual, cloud3) == 0xac, "TPlanetSE.Cloud3ImagePath");
_Static_assert(__builtin_offsetof(PlanetVisual, background) == 0xd0, "TPlanetSE.BackgroundGraph");
_Static_assert(__builtin_offsetof(PlanetVisual, enables_quests) == 0xd4, "TPlanetSE.QuestEnabled");
_Static_assert(sizeof(PlanetTemplate) == 0x10, "TPlanetSpaceTemplate");

typedef void *(DELPHI *FindBlock)(void *, const Wide *);
typedef Byte (DELPHI *FileExists)(void *, const Wide *);
static const FindBlock find_block = (FindBlock)0x7b2eb8;
static const FileExists file_exists = (FileExists)0x4d1c28;
static const Word32 native_factory __attribute__((used)) = 0x782914;
static VisualLog scene_log;

void set_scene_recovery_log(VisualLog log)
{
    scene_log = log;
}

static int starts_with(const Wide *value, const Wide *prefix)
{
    if (!value)
        return 0;
    while (*prefix)
        if (*value++ != *prefix++)
            return 0;
    return 1;
}

static int make_path(ConfigPath *out, const Wide *prefix, const Wide *name, Wide delimiter)
{
    unsigned int length = 0;
    while (*prefix)
        out->text[length++] = *prefix++;
    while (*name && *name != delimiter) {
        if (length >= path_capacity - 1 || *name < 0x20)
            return 0;
        out->text[length++] = *name++;
    }
    out->text[length] = 0;
    out->bytes = length * sizeof(Wide);
    return 1;
}

static int can_read(void *root, const Wide *path)
{
    ConfigPath entry;
    if (starts_with(path, L"GI,"))
        path += 3;
    else if (starts_with(path, L"GAI,"))
        path += 4;
    else if (starts_with(path, L"Alpha,"))
        path += 6;
    return path && *path && make_path(&entry, L"", path, '?') &&
           *entry.text && file_exists(root, entry.text);
}

static int optional_cloud(void *root, const Wide *path)
{
    return !path || !*path || can_read(root, path);
}

static int ready_visual(void *root, const PlanetVisual *visual)
{
    ConfigPath background;
    return can_read(root, visual->texture) && can_read(root, visual->minimap) &&
           optional_cloud(root, visual->cloud1) && optional_cloud(root, visual->cloud2) &&
           optional_cloud(root, visual->cloud3) && visual->background && *visual->background &&
           make_path(&background, L"Bm.PlanetBG.", visual->background, '?') &&
           file_exists(root, background.text);
}

static const Wide *find_replacement(void *root, int radius)
{
    const PlanetTemplate *templates = *(const PlanetTemplate **)0x81cea4;
    if (!templates)
        return 0;
    // The word preceding Delphi's dynamic-array data holds its element count.
    const int count = ((const int *)templates)[-1];
    if (count <= 0 || count > 10000)
        return 0;
    int best_distance = 0x7fffffff;
    const Wide *best = 0;
    for (int i = 0; i < count; ++i) {
        const PlanetTemplate *candidate = &templates[i];
        const PlanetVisual *visual = candidate->visual;
        if (!visual || candidate->style == 1 || candidate->radius <= 0 ||
            candidate->radius > 1024 || visual->enables_quests ||
            !starts_with(visual->key, L"Planet."))
            continue;
        int distance = candidate->radius - radius;
        if (distance < 0)
            distance = -distance;
        if (distance >= best_distance || !ready_visual(root, visual))
            continue;
        if (!distance)
            return visual->key;
        best_distance = distance;
        best = visual->key;
    }
    return best;
}

static void log_replacement(const Wide *prefix, const ConfigPath *missing,
                            const Wide *replacement)
{
    if (!scene_log)
        return;
    const Wide *separator = L" -> ";
    Wide message[path_capacity * 2 + 48];
    unsigned int at = 0;
    while (*prefix)
        message[at++] = *prefix++;
    for (unsigned int i = 0; missing->text[i]; ++i)
        message[at++] = missing->text[i];
    while (*separator)
        message[at++] = *separator++;
    for (unsigned int i = 0; i < path_capacity - 1 && replacement[i]; ++i)
        message[at++] = replacement[i];
    message[at] = 0;
    scene_log(L"FHD_FastCore_Error", message);
}

static const Wide *DELPHI __attribute__((used)) recover_planet_key(
    const Wide *key, const void *buffer, int radius)
{
    if (!buffer || buffer != *(void *volatile *)0x81cc70 || radius < 0 ||
        radius > 65535 || !starts_with(key, L"Planet.") || !key[7] || key[7] == ',')
        return key;
    void *config = *(void *volatile *)0x81cfd8;
    void *resources = *(void *volatile *)0x81cfe4;
    ConfigPath path;
    if (!config || !resources || !make_path(&path, L"SE.", key, ',') ||
        find_block(config, path.text))
        return key;

    const Wide *replacement = find_replacement(resources, radius);
    if (!replacement)
        return key;
    log_replacement(L"Save recovery: missing planet template ", &path, replacement);
    return replacement;
}

void __attribute__((naked)) saved_planet_graph_entry(void)
{
    __asm__ volatile(
        "push %eax\n"
        "push %ecx\n"
        "mov %edx, %eax\n"
        "mov %esi, %edx\n"
        "mov 0x140(%edi), %ecx\n"
        "call recover_planet_key\n"
        "mov %eax, %edx\n"
        "pop %ecx\n"
        "pop %eax\n"
        "jmp *native_factory\n"
    );
}

static const Wide *parameter(void *block, const Wide *key)
{
    const Wide *const *value = find_config_parameter(block, key);
    return value ? *value : 0;
}

static void *available_template(void *config, void *resources, const Wide *key, int station)
{
    ConfigPath path;
    if (!make_path(&path, L"SE.", key, ','))
        return 0;
    void *block = find_block(config, path.text);
    if (!block || !can_read(resources, parameter(block, image.text)) ||
        !can_read(resources, parameter(block, image_map.text)))
        return 0;
    if (station)
        return can_read(resources, parameter(block, image_still.text)) ? block : 0;
    return can_read(resources, parameter(block, image_small.text)) &&
           find_block(block, anim.text) && find_block(block, anim_small.text) ? block : 0;
}

static const Wide *DELPHI __attribute__((used)) recover_ship_key(
    const Wide *key, const void *buffer, unsigned int ship_kind)
{
    static const Wide *const stations[] = {
        center.text, pirate_base.text, military.text, science.text, business.text,
        medical.text, dominion.text
    };
    static const Wide *const ships[] = {
        ranger.text, transport.text, pirate_ship.text, warrior.text, tranclucator.text
    };
    if (!buffer || buffer != *(void *volatile *)0x81cc70)
        return key;
    const int station = starts_with(key, L"Ruins.");
    const unsigned int type = ship_kind & 0xff;
    const Wide *primary;
    const Wide *secondary;
    if (station && type >= station_center && type <= station_custom) {
        if (type == station_custom)
            primary = ship_kind >> 8 == owner_pirate ? pirate_base.text : center.text;
        else
            primary = stations[type - station_center];
        secondary = center.text;
    } else if (!station && type >= 1 && type <= 5 && starts_with(key, L"Ship.")) {
        primary = ships[type - 1];
        secondary = ranger.text;
    } else {
        return key;
    }

    void *config = *(void *volatile *)0x81cfd8;
    void *resources = *(void *volatile *)0x81cfe4;
    ConfigPath path;
    if (!config || !resources || !make_path(&path, L"SE.", key, ',') ||
        find_block(config, path.text))
        return key;
    const Wide *replacement = primary;
    if (!available_template(config, resources, primary, station)) {
        if (secondary == primary || !available_template(config, resources, secondary, station))
            return key;
        replacement = secondary;
    }
    log_replacement(station ? L"Save recovery: missing station template " :
                               L"Save recovery: missing ship template ", &path, replacement);
    return replacement;
}

void __attribute__((naked)) saved_ship_graph_entry(void)
{
    __asm__ volatile(
        "push %eax\n"
        "push %ecx\n"
        "mov %edx, %eax\n"
        "mov %esi, %edx\n"
        "movzwl 0x10(%ebx), %ecx\n"
        "call recover_ship_key\n"
        "mov %eax, %edx\n"
        "pop %ecx\n"
        "pop %eax\n"
        "jmp *native_factory\n"
    );
}

typedef struct {
    const void *vmt;
    Byte before_key[0x14];
    const Wide *key;
} SceneObject;

typedef struct {
    Word32 class_reference;
    const Wide *prefix;
    const Wide *key;
    const Wide *extra_asset;
} SceneFallback;

enum { missile_scene_class = 0x5ccbfc };

typedef void (DELPHI *AssignWide)(const Wide **, const Wide *);
static const FindBlock get_block = (FindBlock)0x7b2dec;
static const AssignWide assign_wide = (AssignWide)0x405d54;

_Static_assert(__builtin_offsetof(SceneObject, key) == 0x18, "TObjectSE.GraphKey");

static const SceneFallback *scene_fallback(const SceneObject *scene, const Wide *path)
{
    static const SceneFallback rules[] = {
        {0x65b880, L"SE.Ship.AkrinFemale.", akrin_female.text, 0},
        {0x65b880, L"SE.Ship.Akrin.", akrin.text, 0},
        {0x65b880, L"SE.Ship.Blazer.", blazer.text, 0},
        {0x65b880, L"SE.Ship.Terron.", terron.text, 0},
        {0x65b880, L"SE.Ship.Keller.", keller.text, 0},
        {0x65b880, L"SE.Ship.", ranger.text, 0},
        {0x6991b0, L"SE.Ruins.", center.text, 0},
        {0x69181c, L"SE.Ruins.", orbital_station.text, 0},
        {0x6c1f54, L"SE.Star.", star_visual.text, anim.text},
        {0x7276d0, L"SE.Hole.", hole_visual.text, 0},
        {0x695cf4, L"SE.Asteroid.", asteroid_visual.text, 0},
        {0x695550, L"SE.Sputnik.", sputnik_visual.text, 0},
        {missile_scene_class, L"SE.Missile.", stock_missile.text, 0}
    };
    for (unsigned int i = 0; i < sizeof(rules) / sizeof(*rules); ++i) {
        const SceneFallback *rule = &rules[i];
        if (scene->vmt == *(const void **)rule->class_reference &&
            starts_with(path, rule->prefix))
            return rule;
    }
    return 0;
}

static void *scene_template(void *config, void *resources, const SceneFallback *rule)
{
    if (rule->class_reference == 0x65b880)
        return available_template(config, resources, rule->key, 0);
    if (rule->class_reference == 0x6991b0 || rule->class_reference == 0x69181c)
        return available_template(config, resources, rule->key, 1);

    ConfigPath path;
    if (!make_path(&path, L"SE.", rule->key, ','))
        return 0;
    void *block = find_block(config, path.text);
    if (!block || !can_read(resources, parameter(block, image.text)))
        return 0;
    if (rule->class_reference != 0x695550 &&
        !can_read(resources, parameter(block, image_map.text)))
        return 0;
    if (rule->extra_asset && !can_read(resources, parameter(block, rule->extra_asset)))
        return 0;
    return block;
}

static int missile_image_available(void *resources, const Wide *key)
{
    ConfigPath image_path;
    return resources && make_path(&image_path, L"Bm.", key, '?') &&
           file_exists(resources, image_path.text);
}

static void *missile_template(void *config, void *resources, const Wide **replacement)
{
    static const Wide *const keys[] = {stock_missile.text, stock_missile_15.text, stock_missile_18.text};
    for (unsigned int i = 0; i < sizeof(keys) / sizeof(*keys); ++i) {
        ConfigPath path;
        if (!make_path(&path, L"SE.", keys[i], '?'))
            continue;
        void *block = find_block(config, path.text);
        if (block && missile_image_available(resources, keys[i])) {
            *replacement = keys[i];
            return block;
        }
    }
    return 0;
}

static void *DELPHI __attribute__((used)) recover_scene_template(
    void *config, const Wide *path, SceneObject *scene)
{
    if (!scene || !config || config != *(void *volatile *)0x81cfd8)
        return get_block(config, path);
    const int missile = scene->vmt == *(const void **)missile_scene_class;
    if (!missile && !*(void *volatile *)0x81cc70)
        return get_block(config, path);
    const SceneFallback *rule = scene_fallback(scene, path);
    if (!rule)
        return get_block(config, path);
    void *block = find_block(config, path);
    if (block)
        return block;
    void *resources = *(void *volatile *)0x81cfe4;
    ConfigPath missing;
    if (!resources || !make_path(&missing, L"", path, ','))
        return get_block(config, path);
    const Wide *replacement = rule->key;
    block = missile ? missile_template(config, resources, &replacement) :
                      scene_template(config, resources, rule);
    if (!block)
        return get_block(config, path);

    // TShip and TPlanet keep a separate original GraphName. Scene-only
    // objects serialize GraphKey itself, so a new save keeps their safe key.
    assign_wide(&scene->key, replacement);
    log_replacement(missile ? L"Missile graphics fallback: " :
                            L"Save recovery: missing scene template ", &missing, replacement);
    return block;
}

void __attribute__((naked)) saved_scene_template_entry(void)
{
    __asm__ volatile("mov %esi, %ecx\n jmp recover_scene_template");
}
