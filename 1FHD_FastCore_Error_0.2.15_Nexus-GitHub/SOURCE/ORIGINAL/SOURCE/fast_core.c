#include "resource_guard.h"
#include "fast_paths.h"
#include "target_map.h"
#include "save_recovery.h"
#include "script_recovery.h"
#include "scene_recovery.h"
#include "orphan_cleanup.h"

#define WINAPI __attribute__((stdcall))
typedef void (WINAPI *HostLog)(const Wide *, const Wide *);
typedef int (WINAPI *ConfigBool)(const Wide *, const Wide *, const Wide *, int);
typedef int (WINAPI *ConfigInt)(const Wide *, const Wide *, const Wide *, int, int, int);

typedef struct {
    Word32 size;
    Word32 api;
    void *game_module;
    const Wide *game_root;
    const Wide *executable;
    const Wide *plugin_root;
    const Wide *config_path;
    HostLog log;
    ConfigBool get_bool;
    ConfigInt get_int;
} Host;

typedef struct {
    Word32 size;
    Word32 required_api;
    Wide id[64];
    Wide version[32];
    Wide description[160];
    unsigned long long exclusive_capabilities;
} Info;

_Static_assert(sizeof(Wide) == 2, "Windows wide characters");
_Static_assert(__builtin_offsetof(Host, log) == 28, "Xeno host log");
_Static_assert(__builtin_offsetof(Host, get_int) == 36, "Xeno config helpers");
_Static_assert(__builtin_offsetof(Info, exclusive_capabilities) == 520, "Xeno query ABI");

extern int WINAPI VirtualProtect(void *, Word32, Word32, Word32 *);
extern int WINAPI FlushInstructionCache(void *, const void *, Word32);
extern int WINAPI GetModuleHandleExW(Word32, const Wide *, void **);

typedef struct {
    Word32 address;
    Word32 length;
    Word32 fingerprint;
} CodeGuard;

typedef struct {
    Word32 address;
    Word32 length;
    Byte replacement[8];
    Byte original[8];
} Patch;

typedef struct {
    Word32 address;
    Word32 old_protection;
} Page;

enum { image_base = 0x400000, page_size = 4096, max_patches = 4 };
static const Wide plugin_id[] = L"FHD_FastCore_Error";
static const Wide section[] = L"FastCore";
static const Word32 rating_resume __attribute__((used)) = 0x7eabee;
static const Word32 wealth_resume __attribute__((used)) = 0x7ea94a;
static const Word32 wealth_done __attribute__((used)) = 0x7ea97e;
static const Word32 strength_resume __attribute__((used)) = 0x7eaae3;
static const Word32 strength_done __attribute__((used)) = 0x7eaae7;
static int initialized;

static unsigned short read16(const Byte *p)
{
    return (unsigned short)(p[0] | ((Word32)p[1] << 8));
}

static Word32 read32(const Byte *p)
{
    return read16(p) | ((Word32)read16(p + 2) << 16);
}

static void write32(Byte *p, Word32 value)
{
    for (unsigned int i = 0; i < 4; ++i)
        p[i] = (Byte)(value >> (8 * i));
}

static int is_target(const Byte *image)
{
    if (image != (const Byte *)image_base || read16(image) != 0x5a4d)
        return 0;
    const Word32 pe_offset = read32(image + 0x3c);
    if (pe_offset < 0x40 || pe_offset > 0x200)
        return 0;
    const Byte *pe = image + pe_offset;
    const Byte *optional = pe + 24;
    return read32(pe) == 0x4550 && read16(pe + 4) == 0x14c &&
           read32(pe + 8) == 0x6ac53b97 && read16(pe + 20) >= 0x60 &&
           read16(optional) == 0x10b && read32(optional + 28) == image_base &&
           read32(optional + 16) == 0x407a08 && read32(optional + 56) == 0x461000;
}

static int guards_match(const CodeGuard *guards, int count)
{
    for (int i = 0; i < count; ++i) {
        const Byte *code = (const Byte *)guards[i].address;
        Word32 fingerprint = 2166136261u;
        for (Word32 j = 0; j < guards[i].length; ++j)
            fingerprint = (fingerprint ^ code[j]) * 16777619u;
        if (fingerprint != guards[i].fingerprint)
            return 0;
    }
    return 1;
}

static void copy_bytes(Byte *to, const Byte *from, Word32 length)
{
    for (Word32 i = 0; i < length; ++i)
        to[i] = from[i];
}

static int unlock_pages(Page *pages, int *count, const Patch *patches, int patch_count)
{
    for (int i = 0; i < patch_count; ++i) {
        const Word32 page = patches[i].address & ~(Word32)(page_size - 1);
        // Every patch in this build's map fits on one page.
        if (patches[i].address - page + patches[i].length > page_size)
            return 0;
        int j = 0;
        while (j < *count && pages[j].address != page)
            ++j;
        if (j == *count) {
            if (!VirtualProtect((void *)page, page_size, 0x40, &pages[j].old_protection))
                return 0;
            pages[j].address = page;
            ++*count;
        }
    }
    return 1;
}

static int restore_pages(const Page *pages, int count)
{
    int ok = 1;
    for (int i = count; i > 0; --i) {
        Word32 ignored;
        const Page *page = &pages[i - 1];
        if (!VirtualProtect((void *)page->address, page_size, page->old_protection, &ignored))
            ok = 0;
    }
    return ok;
}

static int flush_patches(const Patch *patches, int count)
{
    int ok = 1;
    for (int i = 0; i < count; ++i)
        if (!FlushInstructionCache((void *)-1, (const void *)patches[i].address, patches[i].length))
            ok = 0;
    return ok;
}

static void report(const Host *host, const Wide *message)
{
    if (host->log)
        host->log(plugin_id, message);
}

static void apply_group(const Host *host, Patch *patches, int patch_count,
                        const CodeGuard *guards, int guard_count,
                        const Wide *applied, const Wide *conflict)
{
    if (!guards_match(guards, guard_count)) {
        report(host, conflict);
        return;
    }
    Page pages[max_patches];
    int page_count = 0;
    if (!unlock_pages(pages, &page_count, patches, patch_count)) {
        if (!restore_pages(pages, page_count))
            report(host, L"ERROR: could not restore original code page protections");
        report(host, L"Patch skipped: could not unlock all target pages");
        return;
    }

    const Wide *status = conflict;
    int rollback_flushed = 1;
    if (guards_match(guards, guard_count)) {
        for (int i = 0; i < patch_count; ++i) {
            Patch *patch = &patches[i];
            Byte *target = (Byte *)patch->address;
            copy_bytes(patch->original, target, patch->length);
            copy_bytes(target, patch->replacement, patch->length);
        }
        status = applied;
        if (!flush_patches(patches, patch_count)) {
            for (int i = 0; i < patch_count; ++i)
                copy_bytes((Byte *)patches[i].address, patches[i].original, patches[i].length);
            rollback_flushed = flush_patches(patches, patch_count);
            status = L"Patch rolled back: instruction cache flush failed";
        }
    }
    if (!restore_pages(pages, page_count))
        report(host, L"ERROR: could not restore original code page protections");
    if (!rollback_flushed)
        report(host, L"ERROR: instruction cache flush failed after rollback");
    report(host, status);
}

static void branch(Patch *patch, Byte opcode, Word32 destination)
{
    patch->replacement[0] = opcode;
    write32(patch->replacement + 1, destination - patch->address - 5u);
    for (Word32 i = 5; i < patch->length; ++i)
        patch->replacement[i] = 0x90;
}

static void patch_warmup(const Host *host, int days)
{
    static const CodeGuard guard[] = {WARMUP_CODE_GUARD};
    Patch patch = {.address = 0x5bf3f5, .length = 4};
    write32(patch.replacement, (Word32)days + 1u);
    apply_group(host, &patch, 1, guard, 1,
                L"Warmup: configured generation loop limit applied",
                L"Warmup skipped: generation code differs");
}

static void patch_sector_search(const Host *host)
{
    static const CodeGuard guards[] = {
        POLYGON_CONTAINS_GUARD, POLYGON_FIND_GUARD, POLYGON_ASSIGN_GUARD,
        POLYGON_PLACEMENT_GUARD, POLYGON_SEED_GUARD, POLYGON_GROWTH_GUARD
    };
    Patch patches[] = {
        {.address = 0x7e4b66, .length = 5}, {.address = 0x7e4b75, .length = 5},
        {.address = 0x7e3da2, .length = 5}, {.address = 0x7e3f82, .length = 5}
    };
    native_contains_point = (ContainsPoint)0x7cf22c;
    native_find_polygon = (FindPolygon)0x7cf328;
    branch(&patches[0], 0xe8, (Word32)find_polygon_fast);
    branch(&patches[1], 0xe8, (Word32)find_polygon_fast);
    branch(&patches[2], 0xe8, (Word32)assign_group_fast);
    branch(&patches[3], 0xe8, (Word32)assign_group_fast);
    apply_group(host, patches, 4, guards, sizeof(guards) / sizeof(*guards),
                L"Generation: sector polygon search accelerated",
                L"Generation skipped: sector polygon code differs");
}

static void patch_faction_count(const Host *host)
{
    static const CodeGuard guards[] = {
        FACTION_COUNT_GUARD, FACTION_TLIST_GET_GUARD, FACTION_WIDESTRING_COMPARE_GUARD
    };
    Patch patch = {.address = 0x7ead18, .length = 7};
    branch(&patch, 0xe9, (Word32)count_faction_stars_fast);
    apply_group(host, &patch, 1, guards, sizeof(guards) / sizeof(*guards),
                L"Turn calculations: faction star count accelerated",
                L"Faction count skipped: native counter code differs");
}

// Only replace the exchange loop. The native function still owns the array,
// assigns places and performs its achievement check.
static void __attribute__((naked, used)) rank_sort_entry(void)
{
    __asm__ volatile(
        "mov -4(%ebp), %eax\n"
        "mov -8(%ebp), %edx\n"
        "mov 0x3c(%edx), %edx\n"
        "mov 8(%edx), %edx\n"
        "call sort_rangers_fast\n"
        "jmp *rating_resume\n"
    );
}

static void patch_rating(const Host *host)
{
    static const CodeGuard guard[] = {RANGER_RATING_GUARD};
    Patch patch = {.address = 0x7eab81, .length = 6};
    branch(&patch, 0xe9, (Word32)rank_sort_entry);
    apply_group(host, &patch, 1, guard, 1,
                L"Turn calculations: ranger rating sort accelerated",
                L"Ranger rating skipped: native sort code differs");
}

// The native accumulator counts only eligible rangers and pirates. For
// count=0 retain the last usable average or restore the constructor's default.
// The average is itself used as a divisor by other game code.
static void __attribute__((naked, used)) wealth_division_entry(void)
{
    __asm__ volatile(
        "cmpl $0, 4(%esp)\n"
        "je 1f\n"
        "fildll 8(%esp)\n"
        "fildl 4(%esp)\n"
        "jmp *wealth_resume\n"
        "1:\n"
        "cmpl $0, 0x60(%esi)\n"
        "jg 2f\n"
        "movl $3000, 0x60(%esi)\n"
        "2:\n"
        "jmp *wealth_done\n"
    );
}

static void __attribute__((naked, used)) strength_division_entry(void)
{
    __asm__ volatile(
        "cmpl $0, 4(%esp)\n"
        "je 1f\n"
        "fildl 4(%esp)\n"
        "fdivrs 8(%esp)\n"
        "jmp *strength_resume\n"
        "1:\n"
        "mov 0x68(%esi), %eax\n"
        "test %eax, %eax\n"
        "jle 2f\n"
        "cmp $0x7f800000, %eax\n"
        "jb 3f\n"
        "2:\n"
        "movl $0x3f800000, 0x68(%esi)\n"
        "3:\n"
        "jmp *strength_done\n"
    );
}

static void patch_empty_averages(const Host *host)
{
    static const CodeGuard guards[] = {AVERAGE_WEALTH_GUARD, AVERAGE_STRENGTH_GUARD};
    Patch patches[] = {
        {.address = 0x7ea942, .length = 8},
        {.address = 0x7eaadb, .length = 8}
    };
    branch(&patches[0], 0xe9, (Word32)wealth_division_entry);
    branch(&patches[1], 0xe9, (Word32)strength_division_entry);
    apply_group(host, patches, 2, guards, 2,
                L"Turn calculations: zero-participant ranger averages guarded",
                L"Ranger averages skipped: native calculation code differs");
}

// The two audited item-card functions keep the TItem pointer at [EBP-8].
static void __attribute__((naked, used)) item_icon_entry(void)
{
    __asm__ volatile(
        "mov -8(%ebp), %ecx\n"
        "movzbl 12(%ecx), %ecx\n"
        "jmp guarded_item_icon\n"
    );
}

typedef struct {
    Word32 address;
    CodeGuard code;
} VisualSite;

static void patch_visual_group(const Host *host, const VisualSite *sites, int count,
                               Word32 target)
{
    Patch patches[max_patches] = {0};
    CodeGuard guards[max_patches];
    for (int i = 0; i < count; ++i) {
        patches[i].address = sites[i].address;
        patches[i].length = 5;
        guards[i] = sites[i].code;
        branch(&patches[i], 0xe8, target);
    }
    apply_group(host, patches, count, guards, count,
                L"Graphics fallback: guarded load call applied",
                L"Graphics fallback skipped: caller code differs");
}

#define PATCH_VISUAL(sites, target) \
    patch_visual_group(host, sites, sizeof(sites) / sizeof(*(sites)), (Word32)target)

static void patch_item_icons(const Host *host)
{
    static const VisualSite cards[] = ITEM_ICONS_CALLS;
    static const VisualSite shop[] = SHOP_ITEM_IMAGES_CALLS;
    static const VisualSite ship_equipment[] = SHIP_EQUIPMENT_IMAGES_CALLS;
    static const VisualSite ship_hold[] = SHIP_HOLD_IMAGES_CALLS;
    static const VisualSite ship_details[] = SHIP_DETAIL_IMAGES_CALLS;
    static const VisualSite scanner[] = SCANNER_ITEM_IMAGES_CALLS;
    static const VisualSite animations[] = ITEM_ANIMATIONS_CALLS;
    static const VisualSite cursors[] = SHIP_ITEM_CURSORS_CALLS;
    static const VisualSite award_strips[] = AWARD_STRIPS_CALLS;
    static const VisualSite award_hints[] = AWARD_HINTS_CALLS;
    static const VisualSite award_paths[] = AWARD_DISPLAY_PATHS_CALLS;
    static const VisualSite award_text[] = AWARD_TEXT_CALLS;
    PATCH_VISUAL(cards, item_icon_entry);
    PATCH_VISUAL(shop, guarded_item_image);
    PATCH_VISUAL(ship_equipment, guarded_item_image);
    PATCH_VISUAL(ship_hold, guarded_item_image);
    PATCH_VISUAL(ship_details, guarded_item_image);
    PATCH_VISUAL(scanner, guarded_item_image);
    PATCH_VISUAL(animations, guarded_item_animation);
    PATCH_VISUAL(cursors, guarded_item_cursor);
    PATCH_VISUAL(award_strips, guarded_award_icon);
    PATCH_VISUAL(award_hints, guarded_award_icon);
    PATCH_VISUAL(award_paths, guarded_award_path);
    PATCH_VISUAL(award_text, guarded_award_text);
}

static void patch_station_presentation(const Host *host)
{
    static const VisualSite background[] = STATION_BACKGROUND_CALLS;
    static const VisualSite image[] = STATION_IMAGE_CALLS;
    static const VisualSite shops[] = STATION_SHOPS_CALLS;
    static const VisualSite hangar_and_info[] = STATION_HANGAR_INFO_CALLS;
    static const CodeGuard bridge_guards[] = {BRIDGE_HIDE_GUARD, BRIDGE_SELECT_GUARD};
    Patch bridge_patches[] = {{.address = 0x558634, .length = 7},
                              {.address = 0x5586fe, .length = 5}};
    branch(&bridge_patches[0], 0xe9, (Word32)hide_bridge_panels_entry);
    branch(&bridge_patches[1], 0xe9, (Word32)bridge_panel_entry);
    apply_group(host, bridge_patches, 2, bridge_guards, 2,
                L"Save recovery: missing bridge panels guarded",
                L"Bridge panel fallback skipped: caller code differs");
    PATCH_VISUAL(background, guarded_station_background);
    PATCH_VISUAL(image, guarded_station_image);
    PATCH_VISUAL(shops, guarded_station_image);
    PATCH_VISUAL(hangar_and_info, guarded_station_image);
}

static void patch_captains(const Host *host)
{
    static const VisualSite new_game[] = CAPTAIN_NEW_GAME_CALLS;
    static const VisualSite new_game_anim[] = CAPTAIN_NEW_GAME_ANIM_CALLS;
    static const VisualSite talk[] = CAPTAIN_TALK_CALLS;
    static const VisualSite talk_anim[] = CAPTAIN_TALK_ANIM_CALLS;
    static const VisualSite scanner[] = CAPTAIN_SCANNER_CALLS;
    static const VisualSite scanner_anim[] = CAPTAIN_SCANNER_ANIM_CALLS;
    static const VisualSite ship[] = CAPTAIN_SHIP_CALLS;
    static const VisualSite ship_anim[] = CAPTAIN_SHIP_ANIM_CALLS;
    static const VisualSite hangar[] = CAPTAIN_HANGAR_CALLS;
    static const VisualSite hangar_anim[] = CAPTAIN_HANGAR_ANIM_CALLS;
    static const VisualSite market[] = CAPTAIN_MARKET_CALLS;
    static const VisualSite market_anim[] = CAPTAIN_MARKET_ANIM_CALLS;
    static const VisualSite rating[] = CAPTAIN_RATING_CALLS;
    static const VisualSite rating_anim[] = CAPTAIN_RATING_ANIM_CALLS;
    static const VisualSite score[] = CAPTAIN_SCORE_CALLS;
    static const VisualSite score_anim[] = CAPTAIN_SCORE_ANIM_CALLS;
    PATCH_VISUAL(new_game, guarded_portrait);
    PATCH_VISUAL(new_game_anim, guarded_animation);
    PATCH_VISUAL(talk, guarded_portrait);
    PATCH_VISUAL(talk_anim, guarded_animation);
    PATCH_VISUAL(scanner, guarded_portrait);
    PATCH_VISUAL(scanner_anim, guarded_animation);
    PATCH_VISUAL(ship, guarded_portrait);
    PATCH_VISUAL(ship_anim, guarded_animation);
    PATCH_VISUAL(hangar, guarded_portrait);
    PATCH_VISUAL(hangar_anim, guarded_animation);
    PATCH_VISUAL(market, guarded_portrait);
    PATCH_VISUAL(market_anim, guarded_animation);
    PATCH_VISUAL(rating, guarded_portrait);
    PATCH_VISUAL(rating_anim, guarded_animation);
    PATCH_VISUAL(score, guarded_portrait);
    PATCH_VISUAL(score_anim, guarded_animation);
}

static void patch_faction_ratio(const Host *host)
{
    static const CodeGuard guards[] = {
        FACTION_RATIO_GUARD, FACTION_COUNT_GUARD, FACTION_TLIST_GET_GUARD,
        FACTION_WIDESTRING_COMPARE_GUARD
    };
    Patch patch = {.address = 0x7f0b70, .length = 6};
    branch(&patch, 0xe9, (Word32)faction_ratio_fast);
    apply_group(host, &patch, 1, guards, sizeof(guards) / sizeof(*guards),
                L"Turn calculations: coalition and pirates counted in one pass",
                L"Faction ratio skipped: native calculation code differs");
}

static void patch_save_recovery(const Host *host)
{
    static const CodeGuard artefact_guards[] = {
        ART_RECOVERY_LOAD_GUARD, ART_RECOVERY_CONFIG_GUARD, ART_RECOVERY_BITMAP_GUARD
    };
    Patch artefacts[] = {
        {.address = 0x70f842, .length = 5},
        {.address = 0x70e72e, .length = 8}
    };
    set_save_recovery_log(host->log);
    branch(&artefacts[0], 0xe8, (Word32)artefact_config_entry);
    branch(&artefacts[1], 0xe9, (Word32)artefact_bitmap_entry);
    apply_group(host, artefacts, 2, artefact_guards,
                sizeof(artefact_guards) / sizeof(*artefact_guards),
                L"Save recovery: missing custom artefact definitions guarded",
                L"Artefact recovery skipped: native code differs");

    static const CodeGuard module_guards[] = {
        RECOVERY_EXTRA_MODULE_GUARD, RECOVERY_READ_MODULE_GUARD
    };
    Patch extra_module = {.address = 0x6f76aa, .length = 5};
    branch(&extra_module, 0xe9, (Word32)missing_extra_module_entry);
    apply_group(host, &extra_module, 1, module_guards,
                sizeof(module_guards) / sizeof(*module_guards),
                L"Save recovery: unresolved extra micro-module templates guarded",
                L"Extra micro-module recovery skipped: native code differs");

    static const CodeGuard script_guards[] = {
        RECOVERY_SCRIPT_LOOP_GUARD, RECOVERY_SCRIPT_STATE_GUARD,
        RECOVERY_SCRIPT_SERIALIZATION_GUARD, RECOVERY_CACHE_PATH_GUARD,
        RECOVERY_SCRIPT_FINISH_GUARD, RECOVERY_SCRIPT_FINISH_ORIGINAL_GUARD
    };
    Patch scripts[] = {
        {.address = 0x7d2b87, .length = 5},
        {.address = 0x7d2b9e, .length = 7},
        {.address = 0x7d4347, .length = 5}
    };
    set_script_recovery_log(host->log);
    branch(&scripts[0], 0xe8, (Word32)start_script_recovery);
    branch(&scripts[1], 0xe9, (Word32)missing_script_entry);
    branch(&scripts[2], 0xe8, (Word32)finish_script_recovery_entry);
    apply_group(host, scripts, 3, script_guards, sizeof(script_guards) / sizeof(*script_guards),
                L"Save recovery: missing scripts and orphaned bindings guarded",
                L"Script recovery skipped: native code differs");

    static const CodeGuard turn_guards[] = {
        RECOVERY_SCRIPT_TURN_CODE_GUARD, RECOVERY_SCRIPT_VM_GUARD
    };
    Patch script_turn = {.address = 0x64a143, .length = 5};
    branch(&script_turn, 0xe8, (Word32)guarded_script_turn_code);
    apply_group(host, &script_turn, 1, turn_guards, sizeof(turn_guards) / sizeof(*turn_guards),
                L"Save recovery: PC_fem_rangers turn code guarded",
                L"PC_fem_rangers turn guard skipped: native code differs");

    static const CodeGuard planet_guards[] = {
        RECOVERY_PLANET_LOAD_GUARD, RECOVERY_PLANET_FACTORY_GUARD, RECOVERY_GRAPH_CONFIG_GUARD
    };
    Patch planet = {.address = 0x72e19e, .length = 5};
    set_scene_recovery_log(host->log);
    branch(&planet, 0xe8, (Word32)saved_planet_graph_entry);
    apply_group(host, &planet, 1, planet_guards, sizeof(planet_guards) / sizeof(*planet_guards),
                L"Save recovery: missing planet templates guarded",
                L"Planet recovery skipped: native code differs");

    static const CodeGuard station_guards[] = {
        RECOVERY_STATION_VISUAL_GUARD, RECOVERY_SHIP_FRAME_GUARD,
        RECOVERY_PLANET_FACTORY_GUARD, RECOVERY_GRAPH_CONFIG_GUARD
    };
    Patch station = {.address = 0x6c4405, .length = 5};
    branch(&station, 0xe8, (Word32)saved_ship_graph_entry);
    apply_group(host, &station, 1, station_guards,
                sizeof(station_guards) / sizeof(*station_guards),
                L"Save recovery: missing station templates guarded",
                L"Station recovery skipped: native code differs");

    static const CodeGuard ship_guards[] = {
        RECOVERY_SHIP_VISUAL_GUARD, RECOVERY_SHIP_FRAME_GUARD,
        RECOVERY_PLANET_FACTORY_GUARD, RECOVERY_GRAPH_CONFIG_GUARD
    };
    Patch ship = {.address = 0x6c448e, .length = 5};
    branch(&ship, 0xe8, (Word32)saved_ship_graph_entry);
    apply_group(host, &ship, 1, ship_guards, sizeof(ship_guards) / sizeof(*ship_guards),
                L"Save recovery: missing ship templates guarded",
                L"Ship recovery skipped: native code differs");

    static const CodeGuard scene_guards[] = {
        RECOVERY_GRAPH_CONFIG_GUARD, RECOVERY_GRAPH_LOOKUP_GUARD, RECOVERY_SCENE_FACTORY_GUARD,
        RECOVERY_MISSILE_SCENE_FACTORY_GUARD
    };
    Patch scene = {.address = 0x7865fd, .length = 5};
    branch(&scene, 0xe8, (Word32)saved_scene_template_entry);
    apply_group(host, &scene, 1, scene_guards, sizeof(scene_guards) / sizeof(*scene_guards),
                L"Save recovery: missing scene templates guarded",
                L"Scene recovery skipped: native code differs");

    static const CodeGuard message_guards[] = {
        RECOVERY_MESSAGE_CALL_GUARD, RECOVERY_MESSAGE_LOAD_GUARD,
        RECOVERY_GI_SUFFIX_GUARD, RECOVERY_CLEAR_WIDE_GUARD
    };
    Patch message = {.address = 0x5e64e0, .length = 5};
    branch(&message, 0xe8, (Word32)guarded_load_saved_message);
    apply_group(host, &message, 1, message_guards,
                sizeof(message_guards) / sizeof(*message_guards),
                L"Save recovery: missing message icons guarded",
                L"Message icon recovery skipped: native code differs");
}

static int enabled(const Host *host, const Wide *key)
{
    return host->size < 36 || !host->get_bool ||
           host->get_bool(host->config_path, section, key, 1);
}

static void patch_countable_recovery(const Host *host)
{
    static const CodeGuard guards[] = {COUNTABLE_UNIT_SIZE_GUARD, COUNTABLE_MERGE_GUARD};
    Patch patches[] = {{.address = 0x7076be, .length = 5},
                       {.address = 0x7079ca, .length = 8}};
    branch(&patches[0], 0xe9, (Word32)countable_unit_size_entry);
    branch(&patches[1], 0xe9, (Word32)countable_merge_weight_entry);
    apply_group(host, patches, 2, guards, 2,
                L"Save recovery: missing countable definitions guarded",
                L"Countable fallback skipped: native code differs");
}

static void patch_orphan_cleanup(const Host *host, int mode)
{
    static const CodeGuard saved_guards[] = {
        CLEANUP_SAVED_WORLD_GUARD, CLEANUP_SAVED_IMAGE_GUARD, CLEANUP_NATIVE_IMAGE_LOAD_GUARD
    };
    Patch saved[] = {{.address = 0x7d2e07, .length = 7},
                     {.address = 0x7d386f, .length = 5}};
    branch(&saved[0], 0xe9, (Word32)cleanup_saved_world_entry);
    branch(&saved[1], 0xe8, (Word32)cleanup_saved_image_entry);
    set_orphan_cleanup(mode, host->log);
    apply_group(host, saved, 2, saved_guards, sizeof(saved_guards) / sizeof(*saved_guards),
                mode == 2 ? L"Orphan cleanup enabled" : L"Orphan cleanup preview enabled",
                L"Orphan cleanup skipped: save loader code differs");
    if (mode == 2) {
        static const CodeGuard combat_guards[] = {CLEANUP_COMBAT_SHOT_GUARD,
                                                  CLEANUP_COMBAT_CONTINUE_GUARD};
        Patch combat = {.address = 0x7f98d4, .length = 5};
        branch(&combat, 0xe9, (Word32)cleanup_combat_shot_entry);
        apply_group(host, &combat, 1, combat_guards,
                    sizeof(combat_guards) / sizeof(*combat_guards),
                    L"Orphan cleanup: invalid missile shots guarded",
                    L"Orphan shot guard skipped: combat code differs");

        static const CodeGuard missile_scene_guards[] = {
            CLEANUP_MISSILES_OPEN_SCENE_GUARD, CLEANUP_MISSILES_REFRESH_SCENE_GUARD
        };
        Patch missile_scene[] = {{.address = 0x7e2619, .length = 6},
                                 {.address = 0x7e27c1, .length = 6}};
        branch(&missile_scene[0], 0xe9, (Word32)cleanup_missiles_open_scene_entry);
        branch(&missile_scene[1], 0xe9, (Word32)cleanup_missiles_refresh_scene_entry);
        apply_group(host, missile_scene, 2, missile_scene_guards, 2,
                    L"Orphan cleanup: missing missiles removed before drawing space",
                    L"Orphan missile scene cleanup skipped: native code differs");

        static const CodeGuard pickup_guards[] = {
            COUNTABLE_PICKUP_NEAR_GUARD, COUNTABLE_PICKUP_PULL_GUARD, COUNTABLE_PICKUP_NATIVE_GUARD
        };
        Patch pickup[] = {{.address = 0x7fc549, .length = 5},
                          {.address = 0x7fc897, .length = 5}};
        branch(&pickup[0], 0xe8, (Word32)guarded_countable_pickup);
        branch(&pickup[1], 0xe8, (Word32)guarded_countable_pickup);
        apply_group(host, pickup, 2, pickup_guards, 3,
                    L"Orphan cleanup: missing loose item stacks removed at pickup",
                    L"Orphan pickup guard skipped: native code differs");
    }
}

static void copy_wide(Wide *destination, const Wide *source)
{
    do {
        *destination++ = *source;
    } while (*source++);
}

int WINAPI XenoPlugin_Query(Info *out)
{
    if (!out || out->size < 520)
        return 0;
    out->required_api = 1;
    copy_wide(out->id, plugin_id);
    copy_wide(out->version, L"0.2.15");
    copy_wide(out->description, L"Generation, turn and save recovery fixes for Rangers 6AC53B97");
    if (out->size >= sizeof(*out))
        out->exclusive_capabilities = 0;
    return 1;
}

Word32 WINAPI XenoPlugin_Initialize(const Host *host)
{
    if (!host || host->size < 32 || host->api != 1 || !host->game_module)
        return 87;
    if (initialized)
        return 0;
    initialized = 1;
    if (!is_target(host->game_module)) {
        report(host, L"FastCore inactive: expected Rangers.exe build 6AC53B97");
        return 0;
    }
    if (*(void *volatile *)0x81d2a0 || *(void *volatile *)0x81cc54) {
        report(host, L"FastCore inactive: active galaxy or generator; restart the game");
        return 0;
    }

    const int sectors = enabled(host, L"FastSectorSearch");
    const int factions = enabled(host, L"FastFactionCount");
    const int rating = enabled(host, L"FastRangerRating");
    const int averages = enabled(host, L"GuardEmptyRangerAverages");
    const int ratio = enabled(host, L"FastFactionRatio");
    const int icons = enabled(host, L"GuardItemIcons");
    const int captains = enabled(host, L"GuardPilotImages");
    const int recovery = enabled(host, L"GuardSaveRecovery");
    const int has_hooks = sectors || factions || rating || averages || ratio || icons || captains || recovery;
    int days = 60;
    int cleanup = 1;
    if (host->size >= 40 && host->get_int) {
        days = host->get_int(host->config_path, section, L"PresimDays", 60, 60, 300);
        if (recovery)
            cleanup = host->get_int(host->config_path, section, L"OrphanCleanup", 1, 0, 2);
    }
    if (days < 60 || days > 300)
        days = 300;
    if (!has_hooks && days == 300) {
        report(host, L"FastCore inactive: all optimizations disabled");
        return 0;
    }
    if (has_hooks) {
        void *module = 0;
        // FROM_ADDRESS | PIN: installed branches must remain valid until exit.
        if (!GetModuleHandleExW(5, (const Wide *)XenoPlugin_Initialize, &module)) {
            report(host, L"FastCore inactive: could not pin its DLL for the process lifetime");
            return 0;
        }
    }

    if (days != 300)
        patch_warmup(host, days);
    if (sectors)
        patch_sector_search(host);
    if (ratio)
        patch_faction_ratio(host);
    if (factions)
        patch_faction_count(host);
    if (rating)
        patch_rating(host);
    if (averages)
        patch_empty_averages(host);
    set_visual_log(host->log);
    if (icons)
        patch_item_icons(host);
    if (captains)
        patch_captains(host);
    if (recovery) {
        patch_save_recovery(host);
        patch_station_presentation(host);
        patch_countable_recovery(host);
        if (cleanup == 1 || cleanup == 2)
            patch_orphan_cleanup(host, cleanup);
    }
    return 0;
}
