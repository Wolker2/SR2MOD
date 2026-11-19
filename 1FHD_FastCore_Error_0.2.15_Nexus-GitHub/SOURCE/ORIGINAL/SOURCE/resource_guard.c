#include "resource_guard.h"

enum { key_capacity = 192, fallback_size = 32 };
typedef struct {
    Word32 bytes;
    Wide text[key_capacity];
} LocalWideString;

typedef struct {
    void *vmt;
    Word32 width;
    Word32 height;
    Word32 pitch;
    Byte *pixels;
} GraphBuffer;

typedef Byte (DELPHI *FileExists)(void *, const Wide *);
typedef void (DELPHI *LoadIcon)(const Wide *, GraphBuffer *);
typedef void (DELPHI *AllocateImage)(GraphBuffer *, int, int);
typedef void (DELPHI *SetPath)(void *, const Wide *);
typedef void (DELPHI *SetCursor)(void *, const Wide *, const void *);
typedef void (DELPHI *AwardPath)(void *, int, const Wide **);
typedef void (DELPHI *AssignWide)(const Wide **, const Wide *);
typedef void (DELPHI *GetWide)(const Wide **);
typedef void *(DELPHI *FindBlock)(void *, const Wide *);
typedef void (DELPHI *SetActive)(void *, Byte);
typedef const Byte *(DELPHI *GetPlayer)(void);

typedef struct {
    Byte prefix[0x10];
    int kind;
    const Wide *name;
    const Wide *value;
} ConfigEntry;

typedef const ConfigEntry *(DELPHI *FindEntry)(void *, const Wide *, Byte);

static const FileExists file_exists = (FileExists)0x4d1c28;
static const LoadIcon load_icon = (LoadIcon)0x494834;
static const AllocateImage allocate_image = (AllocateImage)0x7b931c;
static const SetPath set_image = (SetPath)0x499014;
static const SetPath set_animation = (SetPath)0x496454;
static const SetCursor set_cursor = (SetCursor)0x4c5d78;
static const SetPath set_cursor_by_name = (SetPath)0x4c5dc4;
static const AwardPath award_path = (AwardPath)0x5d07c4;
static const AssignWide assign_wide = (AssignWide)0x405d54;
static const FindEntry find_entry = (FindEntry)0x7b1f08;
static const FindBlock find_block = (FindBlock)0x7b2eb8;
static const FindBlock find_control = (FindBlock)0x4c5a08;
static const GetWide station_background = (GetWide)0x55754c;
static const GetPlayer get_player = (GetPlayer)0x747300;
static const Word32 hide_bridge_resume __attribute__((used)) = 0x5586e4;
static const Word32 bridge_panel_resume __attribute__((used)) = 0x558745;

static VisualLog visual_log;
static Word32 reported[64];
static unsigned int report_cursor;
static const Wide captain_prefix[] = L"Bm.Captain.2";
static const Wide gi_prefix[] = L"GI,";
static const Wide station_prefix[] = L"Bm.FormRuins.";

void set_visual_log(VisualLog log)
{
    visual_log = log;
}

static int prefix_length(const Wide *key, const Wide *prefix)
{
    int i = 0;
    if (!key)
        return 0;
    while (prefix[i]) {
        if (key[i] != prefix[i])
            return 0;
        ++i;
    }
    return i;
}

static int append(LocalWideString *key, int used, const Wide *part)
{
    while (*part && used < key_capacity - 1)
        key->text[used++] = *part++;
    key->text[used] = 0;
    key->bytes = (Word32)used * 2;
    return used;
}

static void append_byte_number(LocalWideString *path, int used, unsigned int number)
{
    Wide digits[4];
    int start = 3;
    digits[start] = 0;
    do {
        digits[--start] = (Wide)('0' + number % 10);
        number /= 10;
    } while (number && start);
    append(path, used, digits + start);
}

const Wide *const *find_config_parameter(void *block, const Wide *key)
{
    const ConfigEntry *entry = block ? find_entry(block, key, 0) : 0;
    return entry && entry->kind == 1 ? &entry->value : 0;
}

static void report_replacement(const Wide *key, const Wide *reason)
{
    if (!visual_log || !key)
        return;
    Word32 fingerprint = 2166136261u;
    for (int i = 0; key[i]; ++i)
        fingerprint = (fingerprint ^ key[i]) * 16777619u;
    if (!fingerprint)
        fingerprint = 1;
    for (unsigned int i = 0; i < sizeof(reported) / sizeof(*reported); ++i)
        if (reported[i] == fingerprint)
            return;
    reported[report_cursor++ % (sizeof(reported) / sizeof(*reported))] = fingerprint;
    LocalWideString message;
    const int length = append(&message, 0, reason);
    append(&message, length, key);
    visual_log(L"FHD_FastCore_Error", message.text);
}

static int can_read(const Wide *key)
{
    void *root = *(void **)0x81cfe4;
    return root && key && *key && file_exists(root, key);
}

static int can_read_image_key(const Wide *path, int prefix)
{
    LocalWideString key;
    const int length = append(&key, 0, path + prefix);
    return !path[prefix + length] && can_read(key.text);
}

static const Wide *vanilla_item_key(int type)
{
    if (type >= 8 && type <= 41)
        return L"Bm.Items.2ArtRadar_i";
    if (type == 42)
        return L"Bm.Items.2Hull_People_R_i";
    if (type == 71)
        return L"Bm.Micromoduls.2MM2_i";
    return 0;
}

static void draw_fallback(GraphBuffer *graph, int type)
{
    allocate_image(graph, fallback_size, fallback_size);
    if (!graph->pixels || graph->pitch < fallback_size * 4)
        return;
    const Word32 color = type == 71 ? 0xffedba63u : type >= 8 && type <= 41 ?
                         0xffda9acau : 0xff84c8eau;
    for (int y = 0; y < fallback_size; ++y) {
        Word32 *row = (Word32 *)(graph->pixels + y * graph->pitch);
        for (int x = 0; x < fallback_size; ++x) {
            const int frame = x >= 3 && x <= 28 && y >= 3 && y <= 28;
            const int edge = frame && (x == 3 || x == 28 || y == 3 || y == 28);
            const int mark = x >= 14 && x <= 17 &&
                             ((y >= 9 && y <= 18) || (y >= 21 && y <= 23));
            row[x] = edge || mark ? color : frame ? 0xff22252du : 0;
        }
    }
}

static int graph_valid(const GraphBuffer *graph)
{
    return graph->width && graph->height && graph->pixels;
}

static const Wide *checked_gi_path(const Wide *path, LocalWideString *fallback, const Wide *reason)
{
    const int prefix = prefix_length(path, gi_prefix);
    if (!prefix || can_read_image_key(path, prefix))
        return path;
    report_replacement(path + prefix, reason);
    append(fallback, 0, L"GI,Bm.FormMessageBox.2Question");
    return can_read_image_key(fallback->text, prefix) ? fallback->text : 0;
}

void DELPHI guarded_item_image(void *image, const Wide *path)
{
    LocalWideString fallback;
    set_image(image, checked_gi_path(path, &fallback, L"Missing equipment image: "));
}

void DELPHI guarded_item_animation(void *image, const Wide *path)
{
    if (!path || !*path || can_read(path)) {
        set_animation(image, path);
        return;
    }
    report_replacement(path, L"Missing equipment animation: ");
    LocalWideString fallback;
    append(&fallback, 0, L"Bm.ABItem.2_Unknown");
    set_animation(image, can_read(fallback.text) ? fallback.text : 0);
}

void DELPHI guarded_item_cursor(void *loop, const Wide *path, const void *hotspot)
{
    LocalWideString fallback;
    const Wide *safe = checked_gi_path(path, &fallback, L"Missing equipment cursor: ");
    if (safe) {
        set_cursor(loop, safe, hotspot);
    } else {
        append(&fallback, 0, L"Main");
        set_cursor_by_name(loop, fallback.text);
    }
}

void DELPHI guarded_award_icon(const Wide *key, void *destination)
{
    GraphBuffer *graph = destination;
    if (can_read(key)) {
        load_icon(key, graph);
        if (graph_valid(graph))
            return;
        report_replacement(key, L"Empty award image: ");
    } else {
        report_replacement(key, L"Missing award image: ");
    }
    draw_fallback(graph, 0);
}

void DELPHI guarded_award_path(void *screen, int id, const Wide **result)
{
    LocalWideString fallback;
    award_path(screen, id, result);
    const Wide *safe = checked_gi_path(*result, &fallback, L"Missing award image: ");
    if (safe != *result)
        assign_wide(result, safe);
}

void DELPHI guarded_award_text(const Wide *key, const Wide **result)
{
    const Wide *const *value = find_config_parameter(*(void **)0x81cfc0, key);
    if (value) {
        assign_wide(result, *value);
        return;
    }
    report_replacement(key, L"Missing award text: ");
    const int russian = prefix_length(*(const Wide **)0x81cf48, L"russian");
    const Wide *number = key + prefix_length(key, L"Reward.");
    const Wide *end = number;
    while (*end >= '0' && *end <= '9')
        ++end;
    LocalWideString text;
    if (end != number && prefix_length(end, L".Name")) {
        int length = append(&text, 0, russian ? L"Награда №" : L"Award #");
        while (number != end && length < key_capacity - 1)
            text.text[length++] = *number++;
        text.text[length] = 0;
        text.bytes = (Word32)length * sizeof(Wide);
    } else {
        append(&text, 0, russian ? L"Описание недоступно." : L"Description unavailable.");
    }
    assign_wide(result, text.text);
}

static void *bridge_config(void)
{
    void *config = *(void **)0x81cfd8;
    LocalWideString key;
    append(&key, 0, L"CustomBridges");
    return config ? find_block(config, key.text) : 0;
}

static const Wide *bridge_parameter(void *root, unsigned int mode, const Wide *name)
{
    if (!root)
        return 0;
    LocalWideString key;
    append_byte_number(&key, 0, mode);
    void *bridge = find_block(root, key.text);
    append(&key, 0, name);
    const Wide *const *value = find_config_parameter(bridge, key.text);
    return value ? *value : 0;
}

static void *bridge_control(void *screen, void *root, unsigned int mode)
{
    const Wide *name = bridge_parameter(root, mode, L"PanelName");
    return name && *name ? find_control(screen, name) : 0;
}

static void DELPHI __attribute__((used, noinline)) hide_other_bridge_panels(void *screen)
{
    void *root = bridge_config();
    if (!root)
        return;
    const Byte mode = *(const Byte *)0x81b730;
    for (unsigned int i = 1; i <= 255; ++i) {
        if (i == mode)
            continue;
        void *panel = bridge_control(screen, root, i);
        if (panel) {
            const SetActive *vmt = *(const SetActive **)panel;
            vmt[0x28 / sizeof(*vmt)](panel, 0);
        }
    }
}

static void *DELPHI __attribute__((used, noinline)) choose_bridge_panel(void *screen, unsigned int mode)
{
    void *panel = bridge_control(screen, bridge_config(), mode);
    if (panel)
        return panel;
    LocalWideString key;
    const int length = append(&key, 0, L"CustomBridges.");
    append_byte_number(&key, length, mode);
    report_replacement(key.text, L"Missing bridge panel: ");
    append(&key, 0, L"PanelPeopleHolo");
    panel = find_control(screen, key.text);
    if (panel)
        return panel;
    append(&key, 0, L"PanelPeoplePB");
    return find_control(screen, key.text);
}

void __attribute__((naked)) hide_bridge_panels_entry(void)
{
    __asm__ volatile(
        "movl $14, -0x10(%ebp)\n"
        "mov -4(%ebp), %eax\n"
        "call hide_other_bridge_panels\n"
        "jmp *hide_bridge_resume\n"
    );
}

void __attribute__((naked)) bridge_panel_entry(void)
{
    __asm__ volatile(
        "mov %eax, %edx\n"
        "mov -4(%ebp), %eax\n"
        "call choose_bridge_panel\n"
        "jmp *bridge_panel_resume\n"
    );
}

static const Wide *station_fallback(const Byte *player, LocalWideString *key)
{
    const Byte *station = player ? *(const Byte *const *)(player + 0x20) : 0;
    if (station && station[0x11] == 7) {
        append(key, 0, L"Bm.FormRuins.2PBbg");
        if (can_read(key->text))
            return key->text;
    }
    append(key, 0, L"Bm.FormRuins.2RCbg");
    return can_read(key->text) ? key->text : 0;
}

void DELPHI guarded_station_background(const Wide **result)
{
    LocalWideString fallback;
    const Byte mode = *(const Byte *)0x81b730;
    const Byte *player = get_player();
    if (mode) {
        const Wide *saved = player ? *(const Wide *const *)(player + 0xde0) : 0;
        const Wide *safe = saved;
        if (!can_read(safe))
            safe = bridge_parameter(bridge_config(), mode, L"BGI");
        if (can_read(safe)) {
            assign_wide(result, safe);
            return;
        }
        if (saved && *saved)
            report_replacement(saved, L"Missing bridge background: ");
        append(&fallback, 0, L"Bm.FormRuins.2DestroyerBridgebg");
        if (can_read(fallback.text)) {
            assign_wide(result, fallback.text);
            return;
        }
    } else {
        station_background(result);
        if (can_read(*result))
            return;
        report_replacement(*result, L"Missing station background: ");
    }
    assign_wide(result, station_fallback(player, &fallback));
}

void DELPHI guarded_station_image(void *image, const Wide *path)
{
    const int prefix = prefix_length(path, gi_prefix);
    if (prefix && (!path[prefix] || prefix_length(path + prefix, station_prefix)) &&
        !can_read_image_key(path, prefix)) {
        if (path[prefix])
            report_replacement(path + prefix, L"Missing station background: ");
        LocalWideString key, fallback;
        const Wide *safe = station_fallback(get_player(), &key);
        if (safe) {
            const int length = append(&fallback, 0, gi_prefix);
            append(&fallback, length, safe);
        }
        set_image(image, safe ? fallback.text : 0);
        return;
    }
    set_image(image, path);
}

void DELPHI guarded_item_icon(const Wide *key, void *destination, int type)
{
    GraphBuffer *graph = destination;
    if (can_read(key)) {
        load_icon(key, graph);
        if (graph_valid(graph))
            return;
        report_replacement(key, L"Empty item icon: ");
    } else {
        report_replacement(key, L"Missing item icon: ");
        const Wide *fallback = vanilla_item_key(type);
        if (fallback) {
            LocalWideString stock;
            append(&stock, 0, fallback);
            if (can_read(stock.text)) {
                load_icon(stock.text, graph);
                if (graph_valid(graph))
                    return;
            }
        }
    }
    draw_fallback(graph, type);
}

static const Wide *captain_race(const Wide *base)
{
    static const Wide *const names[] = {L"Maloc", L"Peleng", L"People", L"Fei", L"Gaal"};
    const int start = prefix_length(base, captain_prefix);
    for (unsigned int i = 0; i < sizeof(names) / sizeof(*names); ++i) {
        int length = prefix_length(base + start, names[i]);
        if (length && base[start + length] >= '0' && base[start + length] <= '9')
            return names[i];
    }
    return names[2];
}

static void make_captain_path(LocalWideString *out, const Wide *race, int animation)
{
    int used = animation ? 0 : append(out, 0, gi_prefix);
    used = append(out, used, captain_prefix);
    used = append(out, used, race);
    append(out, used, animation ? L"0a" : L"0i");
}

static int make_static_key(LocalWideString *out, const Wide *animation)
{
    int length = append(out, 0, animation);
    if (!length || animation[length] || out->text[length - 1] != 'a')
        return 0;
    out->text[length - 1] = 'i';
    return 1;
}

void DELPHI guarded_portrait(void *image, const Wide *path)
{
    const int prefix = prefix_length(path, gi_prefix);
    if (!prefix || !prefix_length(path + prefix, captain_prefix) || can_read_image_key(path, prefix)) {
        set_image(image, path);
        return;
    }
    report_replacement(path + prefix, L"Missing pilot portrait: ");
    LocalWideString fallback;
    make_captain_path(&fallback, captain_race(path + prefix), 0);
    if (can_read_image_key(fallback.text, prefix)) {
        set_image(image, fallback.text);
        return;
    }
    make_captain_path(&fallback, L"People", 0);
    set_image(image, can_read_image_key(fallback.text, prefix) ? fallback.text : 0);
}

void DELPHI guarded_animation(void *image, const Wide *path)
{
    if (!prefix_length(path, captain_prefix) || can_read(path)) {
        set_animation(image, path);
        return;
    }
    report_replacement(path, L"Missing pilot animation: ");
    LocalWideString fallback;
    if (make_static_key(&fallback, path) && can_read(fallback.text)) {
        // These screens already load CaptainI separately. An empty GAI key
        // makes the native animation control leave that static portrait visible.
        set_animation(image, 0);
        return;
    }
    make_captain_path(&fallback, captain_race(path), 1);
    if (can_read(fallback.text)) {
        set_animation(image, fallback.text);
        return;
    }
    make_captain_path(&fallback, L"People", 1);
    set_animation(image, can_read(fallback.text) ? fallback.text : 0);
}

typedef struct {
    Byte prefix[0x14];
    const Wide *image_override;
} SavedMessage;

typedef void (DELPHI *LoadSavedMessage)(SavedMessage *, void *);
typedef void (DELPHI *ClearWide)(const Wide **);
static const LoadSavedMessage load_saved_message = (LoadSavedMessage)0x7a5540;
static const ClearWide clear_wide = (ClearWide)0x405d18;

void DELPHI guarded_load_saved_message(void *entry, void *buffer)
{
    SavedMessage *message = entry;
    load_saved_message(message, buffer);
    const Wide *override = message->image_override;
    if (!buffer || buffer != *(void *volatile *)0x81cc70 ||
        !override || !*override || !*(void *volatile *)0x81cfe4)
        return;

    static const Wide states[] = L"NAD";
    LocalWideString key;
    int prefix = append(&key, 0, L"Bm.MsgPlayer.2");
    int length = append(&key, prefix, override);
    if (override[length - prefix] || length == key_capacity - 1) {
        report_replacement(key.text, L"Save recovery: invalid message icon name ");
        clear_wide(&message->image_override);
        return;
    }
    key.text[length + 1] = 0;
    key.bytes = (Word32)(length + 1) * sizeof(Wide);
    for (int i = 0; states[i]; ++i) {
        key.text[length] = states[i];
        if (!can_read(key.text)) {
            report_replacement(key.text, L"Save recovery: missing message icon ");
            clear_wide(&message->image_override);
            return;
        }
    }
}
