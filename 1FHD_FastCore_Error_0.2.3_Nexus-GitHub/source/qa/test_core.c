#include "../fast_paths.h"
#include "fixture.h"

#define ST __attribute__((stdcall))
typedef int (DELPHI *AssignGroup)(Polygon *, const Point *, int);
typedef int (DELPHI *CountFaction)(const Galaxy *, Byte);
typedef __WCHAR_TYPE__ Wide;
typedef void (ST *Log)(const Wide *, const Wide *);
typedef int (ST *GetBool)(const Wide *, const Wide *, const Wide *, int);
typedef int (ST *GetInt)(const Wide *, const Wide *, const Wide *, int, int, int);

extern const Byte game_data[], game_end[], plugin_data[], plugin_end[];
extern void *map_fixture(Word32 address, Word32 length);
extern void fail_test(int code) __attribute__((noreturn));
extern void run_rating_loop(const RankedRanger **rangers, const void *galaxy);
extern void rating_return(void);
extern void test_visuals(void);
extern void test_save_recovery(void);
extern void run_wealth_division(int count, unsigned long long total, Word32 *galaxy);
extern void run_strength_division(int count, Word32 total, Word32 *galaxy);

static int mode;
static int successful_logs, pin_calls, writable_pages, protect_calls, flush_calls;
static Word32 measurement[6];
static Word32 ratio_results[128];

static void copy_fixture(Word32 address, const Byte *begin, const Byte *end)
{
    const Word32 length = (Word32)(end - begin);
    Byte *dest = map_fixture(address, length);
    if ((Word32)dest != address)
        fail_test(1);
    for (Word32 i = 0; i < length; ++i)
        dest[i] = begin[i];
}

static int ST protect(void *address, Word32 size, Word32 flags, Word32 *old)
{
    if (((Word32)address & 4095u) || size != 4096)
        fail_test(2);
    ++protect_calls;
    if (flags == 0x40) {
        // Sector patch touches two pages. Force the second unlock to fail.
        if (mode == 3 && (Word32)address == 0x7e3000u)
            return 0;
        *old = 0x20;
        ++writable_pages;
    } else {
        if (flags != 0x20 || --writable_pages < 0)
            fail_test(3);
        *old = 0x40;
    }
    return 1;
}

static int ST flush(void *process, const void *address, Word32 length)
{
    if ((Word32)process != 0xffffffffu || !length || !writable_pages)
        fail_test(4);
    ++flush_calls;
    return mode != 4 || (Word32)address != 0x7e4b66u || flush_calls != 2;
}

static int ST pin(Word32 flags, const Wide *address, void **handle)
{
    if (flags != 5 || (Word32)address != XenoPlugin_Initialize_ADDR)
        fail_test(5);
    ++pin_calls;
    *handle = (void *)DLL_BASE;
    return 1;
}

static void ST log_message(const Wide *id, const Wide *message)
{
    if (id[0] != 'F')
        fail_test(6);
    for (; *message; ++message)
        if (message[0] == 's' && message[1] == 'k' && message[2] == 'i')
            return;
    ++successful_logs;
}

static int ST get_bool(const Wide *path, const Wide *section, const Wide *key, int fallback)
{
    if (path[0] != 'i' || section[0] != 'F' || (key[0] != 'F' && key[0] != 'G') || fallback != 1)
        fail_test(7);
    return mode != 5 && !(mode == 12 && key[0] == 'G' && key[5] == 'S');
}

static int ST get_int(const Wide *path, const Wide *section, const Wide *key,
                      int fallback, int min, int max)
{
    if (path[0] != 'i' || section[0] != 'F' || key[0] != 'P' ||
        fallback != 60 || min != 60 || max != 300)
        fail_test(8);
    return mode == 5 ? 300 : (mode == 7 ? 97 : 60);
}

static Word32 call_target(Word32 address)
{
    const volatile Byte *p = (const volatile Byte *)address;
    Word32 relative = 0;
    for (int i = 0; i < 4; ++i)
        relative |= (Word32)p[i + 1] << (8 * i);
    return address + 5u + relative;
}

enum { cells = 128 };
static Polygon polygon[cells];
static Point vertices[cells][4];
static Point *vertex_ptrs[cells][4];
static struct {
    void *vmt;
    Point **items;
    int count;
} point_lists[cells];

static Word32 bits(float value)
{
    union {float as_float; Word32 as_word;} number = {.as_float = value};
    return number.as_word;
}

static Point point(float x, float y)
{
    return (Point){bits(x), bits(y)};
}

static void setup_polygons(void)
{
    for (int i = 0; i < cells; ++i) {
        const float left = (float)(5 * i);
        vertices[i][0] = point(left, 0);
        vertices[i][1] = point(left + 5, 0);
        vertices[i][2] = point(left + 5, 5);
        vertices[i][3] = point(left, 5);
        for (int j = 0; j < 4; ++j)
            vertex_ptrs[i][j] = &vertices[i][j];
        point_lists[i].items = vertex_ptrs[i];
        point_lists[i].count = 4;
        Polygon *cell = &polygon[i];
        cell->left = bits(left);
        cell->right = bits(left + 5);
        cell->top = bits(0);
        cell->bottom = bits(5);
        cell->group = -1;
        cell->next = i == cells - 1 ? 0 : &polygon[i + 1];
        cell->points = &point_lists[i];
    }
}

static Word32 ticks(void)
{
    Word32 result;
    __asm__ volatile("lfence; rdtsc" : "=a"(result) : : "edx", "memory");
    return result;
}

static void test_polygons(void)
{
    const FindPolygon original = (FindPolygon)0x7cf328u;
    const FindPolygon fast = (FindPolygon)call_target(0x7e4b66u);
    const AssignGroup assign = (AssignGroup)call_target(0x7e3da2u);
    setup_polygons();
    if (original(polygon, &(Point){bits(1), bits(1)}) != polygon ||
        original(polygon, &(Point){bits(5), bits(2)}) != polygon ||
        original(polygon, &(Point){bits(6), bits(2)}) != polygon + 1)
        fail_test(9);
    for (int x = -4; x <= 5 * cells * 2 + 4; ++x)
        for (int y = -2; y <= 12; ++y) {
            Point query = point(x * 0.5f, y * 0.5f);
            if (fast(polygon, &query) != original(polygon, &query))
                fail_test(10);
        }

    Point edge[] = {{0x80000000u, 0}, {0, 0x80000000u},
                    {0x7fc00000u, 0}, {0, 0x7f800000u}};
    for (unsigned int i = 0; i < sizeof(edge) / sizeof(*edge); ++i)
        if (fast(polygon, &edge[i]) != original(polygon, &edge[i]))
            fail_test(11);
    Point query = point(5, 2);
    if (!assign(polygon, &query, 7) || !assign(polygon, &query, 7) ||
        assign(polygon, &query, 8) || polygon[0].group != 7 || polygon[1].group != -1)
        fail_test(12);

    Word32 expected = 0, actual = 0;
    Word32 started = ticks();
    for (int i = 0; i < 20000; ++i) {
        query = point(5 * (i % cells) + 1, 2);
        expected += (Word32)original(polygon, &query);
    }
    measurement[0] = ticks() - started;
    started = ticks();
    for (int i = 0; i < 20000; ++i) {
        query = point(5 * (i % cells) + 1, 2);
        actual += (Word32)fast(polygon, &query);
    }
    measurement[1] = ticks() - started;
    if (expected != actual)
        fail_test(13);
}

static void fill_factions(Star *stars, Star **pointers, int count)
{
    static const Word32 empty[] = {0, 0};
    static const Word32 named[] = {2, 'X'};
    for (int i = 0; i < count; ++i) {
        stars[i].faction = (Byte)(i % 6);
        stars[i].custom_faction = i % 3 == 0 ? 0 : i % 5 == 0 ? empty + 1 : named + 1;
        pointers[i] = &stars[i];
    }
}

static void test_faction_ratio(Galaxy *galaxy, int after)
{
    typedef float (*Ratio)(void);
    const Ratio ratio = (Ratio)0x7f0b70u;
    *(Galaxy **)0x81d2a0u = galaxy;
    for (int i = 0; i < 128; ++i) {
        galaxy->stars->count = i;
        const Word32 value = bits(ratio());
        if (after && value != ratio_results[i])
            fail_test(40);
        ratio_results[i] = value;
    }
    galaxy->stars->count = 128;
    *(Galaxy **)0x81d2a0u = 0;
}

static void test_faction_count(Galaxy *galaxy, int *results, int after)
{
    const CountFaction count = (CountFaction)0x7ead18u;
    if (!after) {
        for (int i = 0; i < 6; ++i)
            results[i] = count(galaxy, (Byte)i);
        if (!results[0] || results[0] <= results[1] || results[1] <= 0)
            fail_test(14);
    }
    Word32 start = ticks();
    for (int i = 0; i < 60000; ++i)
        if (count(galaxy, (Byte)(i % 6)) != results[i % 6])
            fail_test(15);
    measurement[2 + after] = ticks() - start;
    if (after) {
        galaxy->stars->count = 0;
        if (count(galaxy, 0))
            fail_test(18);
    }
}

typedef struct {
    RankedRanger entry;
    Byte reserved[0x510 - sizeof(RankedRanger)];
    unsigned short place;
} TestRanger;
_Static_assert(__builtin_offsetof(TestRanger, place) == 0x510, "rating result");

static void test_ranking(int after)
{
    static TestRanger rangers[100];
    static const RankedRanger *actual[100], *expected[100];
    volatile Byte *end = (volatile Byte *)0x7eac18u;
    Byte saved[5];
    Word32 destination = (Word32)rating_return - (Word32)end - 5u;
    for (int i = 0; i < 5; ++i) {
        saved[i] = end[i];
        end[i] = i ? (Byte)(destination >> (8 * (i - 1))) : 0xe9;
    }
    struct { Word32 unused[2]; int count; } list;
    Word32 galaxy[0x40 / 4] = {0};
    galaxy[0x3c / 4] = (Word32)&list;
    Word32 seed = 2309u;
    for (int trial = 0; trial < 260; ++trial) {
        list.count = trial % 101;
        for (int i = 0; i < list.count; ++i) {
            seed = seed * 1664525u + 1013904223u;
            rangers[i].entry.experience = trial & 1 ? (int)(seed % 9) - 4 : (int)seed;
            rangers[i].place = 0;
            actual[i] = expected[i] = &rangers[i].entry;
        }
        for (int i = 0; i < list.count; ++i)
            for (int j = i; j < list.count; ++j)
                if (expected[i]->experience < expected[j]->experience) {
                    const RankedRanger *swap = expected[i];
                    expected[i] = expected[j];
                    expected[j] = swap;
                }
        run_rating_loop(actual, galaxy);
        for (int i = 0; i < list.count; ++i)
            if (actual[i] != expected[i] || ((const TestRanger *)actual[i])->place != i + 1)
                fail_test(16);
    }
    list.count = 100;
    Word32 start = ticks();
    for (int run = 0; run < 2400; ++run) {
        for (int i = 0; i < list.count; ++i)
            actual[i] = &rangers[i].entry;
        run_rating_loop(actual, galaxy);
    }
    measurement[4 + after] = ticks() - start;
    for (int i = 0; i < 5; ++i)
        end[i] = saved[i];
}

static void test_averages(int after)
{
    Word32 galaxy[0x70 / 4] = {0};
    run_wealth_division(4, 84, galaxy);
    if (galaxy[0x60 / 4] != 21)
        fail_test(19);
    run_wealth_division(1, 0x100000000ULL, galaxy);
    if (galaxy[0x60 / 4] != 0x7fffffffu)
        fail_test(20);
    run_strength_division(4, bits(12.0f), galaxy);
    if (galaxy[0x68 / 4] != bits(3.0f))
        fail_test(21);
    if (!after)
        return;
    galaxy[0x60 / 4] = 100;
    galaxy[0x68 / 4] = bits(100.0f);
    __asm__ volatile("fnclex");
    run_wealth_division(0, 0, galaxy);
    run_strength_division(0, bits(0.0f), galaxy);
    unsigned short status;
    __asm__ volatile("fnstsw %0" : "=am"(status));
    if (galaxy[0x60 / 4] != 100 || galaxy[0x68 / 4] != bits(100.0f) || (status & 5))
        fail_test(29);
    galaxy[0x60 / 4] = 0;
    galaxy[0x68 / 4] = 0x7fc00000u;
    run_wealth_division(0, 0, galaxy);
    run_strength_division(0, 0, galaxy);
    if (galaxy[0x60 / 4] != 3000 || galaxy[0x68 / 4] != bits(1.0f))
        fail_test(33);
}

static void check_uu_bytes(void)
{
    static const Word32 sites[] = {
        0x70d41c, 0x70d4d0, 0x70c244, 0x70c74c, 0x6f9cf0, 0x6f9e54, 0x6d7158,
        0x6fa9cc, 0x6f990c, 0x706990, 0x706aa4, 0x6f9394, 0x70544c, 0x70cb98,
        0x6fa354, 0x6f9788, 0x4c11bc, 0x498fe4, 0x518075, 0x51809b, 0x5182f4,
        0x518321, 0x6a9d77, 0x6a9daa, 0x6a9ff7, 0x6aa024, 0x68039d, 0x6803c9,
        0x680454
    };
    for (unsigned int i = 0; i < sizeof(sites) / sizeof(*sites); ++i) {
        const volatile Byte *code = (const volatile Byte *)sites[i];
        for (int offset = 0; offset < 16; ++offset) {
            const Byte expected = mode == 8 && sites[i] == 0x6f9cf0 && offset == 0 ?
                                  0xe9 : game_data[sites[i] - 0x400000u + offset];
            if (code[offset] != expected)
                fail_test(30);
        }
    }
}

int test_main(const char *argument)
{
    mode = argument[0] - '0';
    if (argument[1])
        mode = mode * 10 + argument[1] - '0';
    copy_fixture(0x400000u, game_data, game_end);
    copy_fixture(DLL_BASE, plugin_data, plugin_end);
    *(Word32 *)VirtualProtect_IAT = (Word32)protect;
    *(Word32 *)FlushInstructionCache_IAT = (Word32)flush;
    *(Word32 *)GetModuleHandleExW_IAT = (Word32)pin;

    struct {
        Word32 size, api, game;
        const Wide *root, *exe, *plugin, *ini;
        Log log;
        GetBool get_bool;
        GetInt get_int;
    } host = {40, 1, 0x400000u, 0, 0, 0, L"ini", log_message, get_bool, get_int};
    int (ST *query)(void *) = (void *)XenoPlugin_Query_ADDR;
    Word32 (ST *initialize)(const void *) = (void *)XenoPlugin_Initialize_ADDR;
    Word32 info[132] = {528};
    if (!query(info) || info[1] != 1 || info[2] != ('F' | ('H' << 16)) ||
        query(0) || initialize(0) != 87)
        fail_test(22);

    if (mode == 1)
        *(Word32 *)0x400108u = 0x68eccc46u;
    if (mode == 2)
        *(Byte *)0x7e4b66u = 0xcc;
    if (mode == 6)
        *(Word32 *)0x81cc54u = 1;
    static Star stars[128];
    static Star *pointers[128];
    if (mode == 8)
        *(Byte *)0x6f9cf0u = 0xe9; // Simulate FHDUU having installed its own hook first.
    if (mode == 9)
        *(Byte *)0x7eaadbu = 0xcc;
    if (mode == 10)
        *(Byte *)0x70e72eu = 0xcc;
    if (mode == 11)
        *(Byte *)0x6f76aau = 0xcc;
    StarList list = {0, pointers, 128};
    Galaxy galaxy = {.stars = &list};
    int expected_factions[6];
    if (mode == 0) {
        fill_factions(stars, pointers, 128);
        test_faction_count(&galaxy, expected_factions, 0);
        test_faction_ratio(&galaxy, 0);
        test_ranking(0);
        test_averages(0);
    }

    if (initialize(&host) || writable_pages)
        fail_test(23);
    if (mode == 1 || mode == 5 || mode == 6) {
        if (protect_calls || flush_calls || pin_calls || *(Word32 *)0x5bf3f5u != 301)
            fail_test(24);
        return 0;
    }
    if (pin_calls != 1 || *(Word32 *)0x5bf3f5u != (mode == 7 ? 98u : 61u) ||
        *(Byte *)0x7ead18u != 0xe9 || *(Byte *)0x7eab81u != 0xe9)
        fail_test(25);
    if (mode == 2 || mode == 3 || mode == 4) {
        if (*(Byte *)0x7e4b66u != (mode == 2 ? 0xcc : 0xe8) ||
            call_target(0x7e4b75u) != 0x7cf328u || call_target(0x7e3da2u) != 0x7cf358u ||
            call_target(0x7e3f82u) != 0x7cf358u)
            fail_test(26);
        return 0;
    }
    check_uu_bytes();
    if (mode == 9) {
        if (*(Byte *)0x7ea942u != 0xdf || *(Byte *)0x7eaadbu != 0xcc)
            fail_test(31);
        return 0;
    }
    if (*(Byte *)0x7ea942u != 0xe9 || *(Byte *)0x7eaadbu != 0xe9)
        fail_test(32);
    if (mode >= 10) {
        if ((call_target(0x70f842u) >= DLL_BASE) != (mode == 11) ||
            (*(Byte *)0x70e72eu == 0xe9) != (mode == 11) ||
            (*(Byte *)0x6f76aau == 0xe9) != (mode == 10))
            fail_test(33);
        return 0;
    }
    if (call_target(0x7e4b66u) < DLL_BASE || call_target(0x7e3f82u) < DLL_BASE ||
        successful_logs != 23)
        fail_test(27);
    int previous = protect_calls;
    if (initialize(&host) || protect_calls != previous)
        fail_test(28);
    if (mode == 0) {
        test_polygons();
        test_faction_ratio(&galaxy, 1);
        test_faction_count(&galaxy, expected_factions, 1);
        test_visuals();
        test_save_recovery();
        test_ranking(1);
        test_averages(1);
        for (int i = 0; i < 6; ++i)
            ((volatile Word32 *)0x30000008u)[i] = measurement[i];
    }
    return 0;
}
