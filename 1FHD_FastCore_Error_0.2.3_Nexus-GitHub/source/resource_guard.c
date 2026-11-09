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

static const FileExists file_exists = (FileExists)0x4d1c28;
static const LoadIcon load_icon = (LoadIcon)0x494834;
static const AllocateImage allocate_image = (AllocateImage)0x7b931c;
static const SetPath set_portrait = (SetPath)0x499014;
static const SetPath set_animation = (SetPath)0x496454;

static VisualLog visual_log;
static Word32 reported[64];
static unsigned int report_cursor;
static const Wide captain_prefix[] = L"Bm.Captain.2";
static const Wide gi_prefix[] = L"GI,";

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

static int can_read_portrait_key(const Wide *path, int prefix)
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
    if (!prefix || !prefix_length(path + prefix, captain_prefix) || can_read_portrait_key(path, prefix)) {
        set_portrait(image, path);
        return;
    }
    report_replacement(path + prefix, L"Missing pilot portrait: ");
    LocalWideString fallback;
    make_captain_path(&fallback, captain_race(path + prefix), 0);
    if (can_read_portrait_key(fallback.text, prefix)) {
        set_portrait(image, fallback.text);
        return;
    }
    make_captain_path(&fallback, L"People", 0);
    set_portrait(image, can_read_portrait_key(fallback.text, prefix) ? fallback.text : 0);
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
