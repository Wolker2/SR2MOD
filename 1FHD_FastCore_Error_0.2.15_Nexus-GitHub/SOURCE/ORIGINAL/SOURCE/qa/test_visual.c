#include "../resource_guard.h"
#include "fixture.h"

typedef struct {
    Word32 bytes;
    Wide value[100];
} TestKey;

typedef struct {
    void *vmt;
    Word32 width, height, pitch;
    Byte *pixels;
} TestGraph;

extern void fail_test(int code) __attribute__((noreturn));
extern void run_item_load(const Wide *path, void *graph, const void *item, Word32 function);
static const Wide *readable[8];
static Wide observed[100];
static Word32 pixels[32 * 32];
static int loads, setters, invalid_image;
static TestKey generated_award, assigned_award;
static const int hotspot[] = {16, 16};

static int equals(const Wide *a, const Wide *b)
{
    if (!a || !b)
        return a == b;
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return *a == *b;
}

static const Wide *key(TestKey *out, const Wide *path)
{
    int n = 0;
    while (path[n]) {
        out->value[n] = path[n];
        ++n;
    }
    out->value[n] = 0;
    out->bytes = n * 2u;
    return out->value;
}

static void record(const Wide *path)
{
    if (!path) {
        observed[0] = 0;
        return;
    }
    int n = 0;
    while (path[n] && n < 98) {
        observed[n] = path[n];
        ++n;
    }
    observed[n] = 0;
    if (((const Word32 *)path)[-1] != n * 2u)
        fail_test(53);
}

static Byte DELPHI mock_read(void *root, const Wide *path)
{
    if (root != (void *)0x431000u)
        fail_test(50);
    record(path);
    for (int i = 0; readable[i]; ++i)
        if (equals(readable[i], path))
            return 1;
    return 0;
}

static void DELPHI mock_allocate(TestGraph *graph, int width, int height)
{
    graph->width = width;
    graph->height = height;
    graph->pitch = width * 4;
    graph->pixels = (Byte *)pixels;
}

static void DELPHI mock_load(const Wide *path, TestGraph *graph)
{
    record(path);
    ++loads;
    mock_allocate(graph, invalid_image ? 0 : 16, 16);
}

static void DELPHI mock_set(void *control, const Wide *path)
{
    if (control != (void *)0x430000u)
        fail_test(51);
    record(path);
    ++setters;
}

static void DELPHI mock_set_cursor(void *control, const Wide *path, const void *point)
{
    if (point != hotspot)
        fail_test(169);
    mock_set(control, path);
}

static void DELPHI mock_award_path(void *screen, int id, const Wide **result)
{
    if (screen != (void *)0x430000u || id != 68)
        fail_test(170);
    *result = key(&generated_award, L"GI,Bm.FormRewards.2_68");
}

static void DELPHI mock_assign_wide(const Wide **destination, const Wide *path)
{
    record(path);
    *destination = path ? key(&assigned_award, path) : 0;
}

static void branch(Word32 site, Word32 destination)
{
    volatile Byte *instruction = (volatile Byte *)site;
    Word32 relative = destination - site - 5u;
    instruction[0] = 0xe9;
    for (int i = 0; i < 4; ++i)
        instruction[i + 1] = (Byte)(relative >> (i * 8));
}

static Word32 destination(Word32 site)
{
    const volatile Byte *instruction = (const volatile Byte *)site;
    if (instruction[0] != 0xe8)
        fail_test(52);
    Word32 relative = 0;
    for (int i = 0; i < 4; ++i)
        relative |= (Word32)instruction[i + 1] << (i * 8);
    return site + 5u + relative;
}

void test_visuals(void)
{
    const Word32 card = destination(0x58e994u);
    const Word32 equipment = destination(0x5978b9u);
    const Word32 image = destination(0x5c73acu);
    const Word32 animation = destination(0x5c7474u);
    if (card < DLL_BASE || equipment != card || image < DLL_BASE || animation < DLL_BASE)
        fail_test(54);
    *(void **)0x81cfe4u = (void *)0x431000u;
    branch(0x4d1c28u, (Word32)mock_read);
    branch(0x494834u, (Word32)mock_load);
    branch(0x7b931cu, (Word32)mock_allocate);
    branch(0x499014u, (Word32)mock_set);
    branch(0x496454u, (Word32)mock_set);

    TestGraph graph = {0};
    Byte item[16] = {0};
    TestKey input;
    item[12] = 71;
    readable[0] = L"Bm.Items.2Original_i";
    run_item_load(key(&input, readable[0]), &graph, item, card);
    if (loads != 1 || graph.width != 16 || !equals(observed, readable[0]))
        fail_test(55);

    readable[0] = L"Bm.Micromoduls.2MM2_i";
    run_item_load(key(&input, L"Bm.Items.2BrokenMM_i"), &graph, item, equipment);
    if (loads != 2 || graph.width != 16 || !equals(observed, readable[0]))
        fail_test(56);

    item[12] = 20;
    readable[0] = L"Bm.Items.2ArtRadar_i";
    run_item_load(key(&input, L"Bm.Items.2BrokenArtifact_i"), &graph, item, card);
    if (loads != 3 || graph.width != 16 || !equals(observed, readable[0]))
        fail_test(57);

    readable[0] = 0;
    run_item_load(key(&input, L"Bm.Items.2Missing_i"), &graph, item, card);
    if (loads != 3 || graph.width != 32 || graph.height != 32 || !pixels[3 * 32 + 3])
        fail_test(58);

    invalid_image = 1;
    readable[0] = L"Bm.Items.2Zero_i";
    run_item_load(key(&input, readable[0]), &graph, item, card);
    if (loads != 4 || graph.width != 32)
        fail_test(59);

    typedef void (DELPHI *SetVisual)(void *, const Wide *);
    const SetVisual set_image = (SetVisual)image;
    const SetVisual set_anim = (SetVisual)animation;
    void *control = (void *)0x430000u;
    readable[0] = L"Bm.Captain.2Peleng17i";
    set_image(control, key(&input, L"GI,Bm.Captain.2Peleng17i"));
    if (setters != 1 || !equals(observed, L"GI,Bm.Captain.2Peleng17i"))
        fail_test(60);
    set_anim(control, key(&input, L"Bm.Captain.2Peleng17a"));
    if (setters != 2 || observed[0])
        fail_test(61);

    readable[0] = L"Bm.Captain.2Peleng0i";
    set_image(control, key(&input, L"GI,Bm.Captain.2Peleng17i"));
    if (!equals(observed, L"GI,Bm.Captain.2Peleng0i"))
        fail_test(62);
    readable[0] = L"Bm.Captain.2Peleng0a";
    set_anim(control, key(&input, L"Bm.Captain.2Peleng17a"));
    if (!equals(observed, L"Bm.Captain.2Peleng0a"))
        fail_test(63);
    readable[0] = L"Bm.Captain.2People0i";
    set_image(control, key(&input, L"GI,Bm.Captain.2Peleng17i"));
    if (!equals(observed, L"GI,Bm.Captain.2People0i"))
        fail_test(64);
    readable[0] = 0;
    set_anim(control, key(&input, L"Bm.Captain.2Unknown99a"));
    if (observed[0])
        fail_test(65);

    // A non-captain image at a shared call site must pass through untouched.
    set_image(control, key(&input, L"GI,Bm.FormOther.Icon"));
    if (!equals(observed, L"GI,Bm.FormOther.Icon"))
        fail_test(66);

    const SetVisual set_equipment = (SetVisual)destination(0x68025cu);
    const SetVisual set_preview = (SetVisual)destination(0x68033au);
    if (destination(0x6a5899u) != image || destination(0x6a590au) != animation ||
        destination(0x6ab18bu) != (Word32)set_equipment ||
        destination(0x518251u) != (Word32)set_equipment)
        fail_test(171);

    readable[0] = L"Bm.Items.2Wphoton_guni";
    set_equipment(control, key(&input, L"GI,Bm.Items.2Wphoton_guni"));
    if (!equals(observed, L"GI,Bm.Items.2Wphoton_guni"))
        fail_test(172);
    readable[0] = L"Bm.FormMessageBox.2Question";
    set_equipment(control, key(&input, L"GI,Bm.Items.2Wphoton_guni"));
    if (!equals(observed, L"GI,Bm.FormMessageBox.2Question"))
        fail_test(173);
    branch(0x4c5d78u, (Word32)mock_set_cursor);
    branch(0x4c5dc4u, (Word32)mock_set);
    typedef void (DELPHI *SetCursor)(void *, const Wide *, const void *);
    const SetCursor set_cursor = (SetCursor)destination(0x6ad877u);
    set_cursor(control, key(&input, L"GI,Bm.Items.2Wphoton_guns"), hotspot);
    if (!equals(observed, L"GI,Bm.FormMessageBox.2Question"))
        fail_test(174);
    readable[0] = 0;
    set_cursor(control, key(&input, L"GI,Bm.Items.2Wphoton_guns"), hotspot);
    if (!equals(observed, L"Main"))
        fail_test(175);
    set_equipment(control, key(&input, L"GI,Bm.Items.2Wphoton_guni"));
    if (observed[0])
        fail_test(176);

    readable[0] = L"Bm.ABItem.2_Unknown";
    set_preview(control, key(&input, L"Bm.Items.2Wphoton_guna"));
    if (!equals(observed, readable[0]))
        fail_test(177);
    readable[0] = L"Bm.Items.2Wphoton_guna";
    set_preview(control, key(&input, readable[0]));
    if (!equals(observed, readable[0]))
        fail_test(178);
    readable[0] = 0;
    set_preview(control, key(&input, L"Bm.Items.2Wphoton_guna"));
    if (observed[0])
        fail_test(179);

    typedef void (DELPHI *LoadAward)(const Wide *, void *);
    const LoadAward load_award = (LoadAward)destination(0x5d530cu);
    if (destination(0x5d3fa9u) != (Word32)load_award ||
        destination(0x6a7309u) != (Word32)load_award ||
        destination(0x515d5bu) != (Word32)load_award)
        fail_test(180);
    invalid_image = 0;
    loads = 0;
    readable[0] = L"Bm.FormRewards.2_08";
    load_award(key(&input, readable[0]), &graph);
    if (loads != 1 || graph.width != 16 || !equals(observed, readable[0]))
        fail_test(181);
    load_award(key(&input, L"Bm.FormRewards.2_68"), &graph);
    if (loads != 1 || graph.width != 32 || graph.height != 32 || !pixels[3 * 32 + 3])
        fail_test(182);
    invalid_image = 1;
    load_award(key(&input, readable[0]), &graph);
    if (loads != 2 || graph.width != 32)
        fail_test(183);

    typedef void (DELPHI *GetAwardPath)(void *, int, const Wide **);
    const GetAwardPath get_award_path = (GetAwardPath)destination(0x5d12f3u);
    volatile Byte *wide_assign_code = (volatile Byte *)0x405d54u;
    Byte native_assign[5];
    for (int i = 0; i < 5; ++i)
        native_assign[i] = wide_assign_code[i];
    branch(0x5d07c4u, (Word32)mock_award_path);
    branch(0x405d54u, (Word32)mock_assign_wide);
    const Wide *result = 0;
    readable[0] = L"Bm.FormRewards.2_68";
    get_award_path(control, 68, &result);
    if (result != generated_award.value)
        fail_test(184);
    readable[0] = L"Bm.FormMessageBox.2Question";
    get_award_path(control, 68, &result);
    if (result != assigned_award.value || !equals(result, L"GI,Bm.FormMessageBox.2Question"))
        fail_test(185);
    readable[0] = 0;
    get_award_path(control, 68, &result);
    if (result)
        fail_test(186);
    for (int i = 0; i < 5; ++i)
        wide_assign_code[i] = native_assign[i];
}

typedef struct {
    Byte prefix[0x14];
    const Wide *image;
    Word32 text_and_target;
} MessageFixture;

extern void run_saved_message(MessageFixture *, void *);
extern void finish_saved_message(void);

static const Wide *message_image;
static const Wide *absent_icon;
static int message_buffer, icon_checks, cleared;

static void DELPHI load_message(MessageFixture *message, void *buffer)
{
    if (buffer != &message_buffer)
        fail_test(163);
    message->image = message_image;
    message->text_and_target = 0x13572468;
}

static void DELPHI clear_message_image(const Wide **image)
{
    if (!*image)
        fail_test(164);
    ++cleared;
    *image = 0;
}

static Byte DELPHI message_icon_exists(void *root, const Wide *key)
{
    static const Wide prefix[] = L"Bm.MsgPlayer.2";
    if (root != (void *)0x431000)
        fail_test(165);
    unsigned int length = 0;
    while (key[length])
        ++length;
    if (((const Word32 *)key)[-1] != length * 2)
        fail_test(166);
    for (unsigned int i = 0; prefix[i]; ++i)
        if (key[i] != prefix[i])
            fail_test(167);
    ++icon_checks;
    return !absent_icon || !equals(key, absent_icon);
}

static void check_saved_message(const Wide *name, const Wide *missing, int in_file,
                                int resource_root, int expected_checks, int clear_expected)
{
    message_image = name;
    absent_icon = missing;
    *(void **)0x81cc70u = in_file ? &message_buffer : 0;
    *(void **)0x81cfe4u = resource_root ? (void *)0x431000 : 0;
    icon_checks = cleared = 0;
    MessageFixture message = {0};
    run_saved_message(&message, &message_buffer);
    if (icon_checks != expected_checks || cleared != clear_expected ||
        message.image != (clear_expected ? 0 : name) || message.text_and_target != 0x13572468)
        fail_test(168);
}

void test_saved_messages(void)
{
    branch(0x7a5540, (Word32)load_message);
    branch(0x405d18, (Word32)clear_message_image);
    branch(0x4d1c28, (Word32)message_icon_exists);
    branch(0x5e64e5, (Word32)finish_saved_message);
    check_saved_message(L"MMStorage", L"Bm.MsgPlayer.2MMStorageN", 1, 1, 1, 1);
    check_saved_message(L"MMStorage", L"Bm.MsgPlayer.2MMStorageA", 1, 1, 2, 1);
    check_saved_message(L"MMStorage", L"Bm.MsgPlayer.2MMStorageD", 1, 1, 3, 1);
    check_saved_message(L"InstalledMod", 0, 1, 1, 3, 0);
    check_saved_message(L"", 0, 1, 1, 0, 0);
    check_saved_message(L"MMStorage", L"Bm.MsgPlayer.2MMStorageN", 0, 1, 0, 0);
    check_saved_message(L"MMStorage", L"Bm.MsgPlayer.2MMStorageN", 1, 0, 0, 0);
    Wide overlong[193];
    for (unsigned int i = 0; i < 192; ++i)
        overlong[i] = 'a';
    overlong[192] = 0;
    check_saved_message(overlong, 0, 1, 1, 0, 1);
}

typedef struct {
    Byte prefix[0x10];
    int kind;
    const Wide *name, *value;
} TestConfigEntry;

extern void *run_bridge_panel(void *, unsigned int);
extern int run_hide_bridge_panels(void *);
extern void finish_bridge_panel(void);
extern void finish_hide_bridge_panels(void);

static char game_config, language_config, bridges, bridge_1, bridge_3;
static Byte player[0xde4], station[0x12];
static TestKey bridge_image, bridge_name, saved_background, native_background, title;
static int have_bridges, have_custom, have_custom_panel, holo_hidden, custom_hidden;
static int native_background_calls;

typedef void (DELPHI *ActivatePanel)(void *, Byte);
static ActivatePanel panel_vmt[11];
static struct { ActivatePanel *vmt; } holo_panel = {panel_vmt}, custom_panel = {panel_vmt};

static void DELPHI mock_panel_active(void *panel, Byte active)
{
    if (active)
        fail_test(189);
    if (panel == &holo_panel)
        ++holo_hidden;
    else if (panel == &custom_panel)
        ++custom_hidden;
    else
        fail_test(190);
}

static void *DELPHI mock_bridge_block(void *block, const Wide *name)
{
    record(name);
    if (block == &game_config && equals(name, L"CustomBridges"))
        return have_bridges ? &bridges : 0;
    if (block != &bridges)
        fail_test(191);
    if (equals(name, L"1"))
        return &bridge_1;
    if (equals(name, L"3") && have_custom)
        return &bridge_3;
    return 0;
}

static TestConfigEntry *DELPHI mock_config_parameter(void *block, const Wide *name, Byte raises)
{
    if (raises)
        fail_test(192);
    record(name);
    static TestConfigEntry entry = {.kind = 1};
    if (block == &language_config) {
        if (equals(name, L"Reward.49.Name")) {
            entry.value = key(&title, L"Existing award");
            return &entry;
        }
        if (equals(name, L"Reward.49.Text")) {
            entry.value = 0;
            return &entry;
        }
        return 0;
    }
    if (block == &bridge_1 && equals(name, L"PanelName")) {
        entry.value = key(&bridge_name, L"PanelPeopleHolo");
        return &entry;
    }
    if (block != &bridge_3)
        fail_test(193);
    if (equals(name, L"PanelName")) {
        entry.value = key(&bridge_name, L"PanelCustom");
        return &entry;
    }
    if (equals(name, L"BGI")) {
        entry.value = key(&bridge_image, L"Bm.FormRuins.2CustomBridgebg");
        return &entry;
    }
    return 0;
}

static void *DELPHI mock_bridge_control(void *screen, const Wide *name)
{
    if (screen != (void *)0x430000u)
        fail_test(194);
    record(name);
    if (equals(name, L"PanelPeopleHolo"))
        return &holo_panel;
    if (equals(name, L"PanelCustom") && have_custom_panel)
        return &custom_panel;
    return 0;
}

static const Byte *DELPHI mock_player(void)
{
    return player;
}

static void DELPHI mock_station_background(const Wide **result)
{
    ++native_background_calls;
    *result = key(&native_background, L"Bm.FormRuins.2UBbg");
}

void test_station_and_award_text(void)
{
    branch(0x405d54u, (Word32)mock_assign_wide);
    branch(0x4d1c28u, (Word32)mock_read);
    branch(0x7b2eb8u, (Word32)mock_bridge_block);
    branch(0x7b1f08u, (Word32)mock_config_parameter);
    branch(0x4c5a08u, (Word32)mock_bridge_control);
    branch(0x747300u, (Word32)mock_player);
    branch(0x55754cu, (Word32)mock_station_background);
    branch(0x558745u, (Word32)finish_bridge_panel);
    branch(0x5586e4u, (Word32)finish_hide_bridge_panels);
    *(void **)0x81cfe4u = (void *)0x431000u;
    *(void **)0x81cfd8u = &game_config;
    *(void **)0x81cfc0u = &language_config;
    *(void **)(player + 0x20) = station;
    panel_vmt[10] = mock_panel_active;
    const Wide *result = 0;
    TestKey input;
    void *screen = (void *)0x430000u;
    typedef void (DELPHI *TextLookup)(const Wide *, const Wide **);
    const TextLookup lookup = (TextLookup)destination(0x742e12u);
    if (destination(0x742e57u) != (Word32)lookup)
        fail_test(195);
    lookup(key(&input, L"Reward.49.Name"), &result);
    if (!equals(result, L"Existing award"))
        fail_test(196);
    lookup(key(&input, L"Reward.49.Text"), &result);
    if (result)
        fail_test(197);
    *(const Wide **)0x81cf48u = L"russian";
    lookup(key(&input, L"Reward.68.Name"), &result);
    if (!equals(result, L"Награда №68"))
        fail_test(198);
    lookup(key(&input, L"Reward.58.Text"), &result);
    if (!equals(result, L"Описание недоступно."))
        fail_test(199);
    *(const Wide **)0x81cf48u = L"english";
    lookup(key(&input, L"Reward.255.Name"), &result);
    if (!equals(result, L"Award #255"))
        fail_test(200);

    *(Byte *)0x81b730u = 3;
    player[0xdd0] = 3;
    have_bridges = have_custom = have_custom_panel = 1;
    if (run_hide_bridge_panels(screen) != 14 || holo_hidden != 1 || custom_hidden ||
        run_bridge_panel(screen, 3) != &custom_panel)
        fail_test(201);
    have_custom_panel = 0;
    if (run_bridge_panel(screen, 3) != &holo_panel)
        fail_test(202);
    have_custom = 0;
    if (run_bridge_panel(screen, 3) != &holo_panel)
        fail_test(203);
    have_bridges = 0;
    if (run_hide_bridge_panels(screen) != 14 || holo_hidden != 1 ||
        run_bridge_panel(screen, 3) != &holo_panel)
        fail_test(204);

    typedef void (DELPHI *GetBackground)(const Wide **);
    const GetBackground background = (GetBackground)destination(0x55854bu);
    readable[0] = L"Bm.FormRuins.2DestroyerBridgebg";
    background(&result);
    if (!equals(result, readable[0]) || native_background_calls ||
        player[0xdd0] != 3 || *(Byte *)0x81b730u != 3)
        fail_test(205);
    have_bridges = have_custom = 1;
    readable[0] = L"Bm.FormRuins.2CustomBridgebg";
    const Wide *saved = key(&saved_background, L"Bm.FormRuins.2SavedBridgebg");
    *(const Wide **)(player + 0xde0) = saved;
    background(&result);
    if (!equals(result, readable[0]) || *(const Wide **)(player + 0xde0) != saved)
        fail_test(206);
    readable[0] = saved;
    background(&result);
    if (!equals(result, saved))
        fail_test(207);

    *(Byte *)0x81b730u = 0;
    readable[0] = L"Bm.FormRuins.2UBbg";
    background(&result);
    if (!equals(result, readable[0]) || native_background_calls != 1)
        fail_test(208);
    readable[0] = L"Bm.FormRuins.2RCbg";
    background(&result);
    if (!equals(result, readable[0]) || native_background_calls != 2)
        fail_test(209);
    station[0x11] = 7;
    readable[0] = L"Bm.FormRuins.2PBbg";
    background(&result);
    if (!equals(result, readable[0]))
        fail_test(210);
    typedef void (DELPHI *SetImage)(void *, const Wide *);
    const SetImage set_background = (SetImage)destination(0x67f27bu);
    set_background(screen, key(&input, L"GI,Bm.FormRuins.2UBbg"));
    if (!equals(observed, L"GI,Bm.FormRuins.2PBbg"))
        fail_test(211);
    readable[0] = 0;
    set_background(screen, key(&input, L"GI,"));
    if (observed[0])
        fail_test(212);
}
