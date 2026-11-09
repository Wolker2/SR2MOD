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
}
