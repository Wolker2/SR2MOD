#include "fast_paths.h"

#define OFFSET(Type, member) __builtin_offsetof(Type, member)
_Static_assert(OFFSET(Polygon, left) == 0x28 && OFFSET(Polygon, bottom) == 0x34, "polygon bounds");
_Static_assert(OFFSET(Polygon, next) == 0x04 && OFFSET(Polygon, group) == 0x10, "polygon links");
_Static_assert(OFFSET(Galaxy, stars) == 0x2c, "galaxy list");
_Static_assert(OFFSET(StarList, items) == 0x04 && OFFSET(StarList, count) == 0x08, "TList");
_Static_assert(OFFSET(Star, faction) == 0x42 && OFFSET(Star, custom_faction) == 0x44, "star faction");

ContainsPoint native_contains_point;
FindPolygon native_find_polygon;

static int positive_finite(Word32 bits)
{
    return bits < 0x7f800000u;
}

Polygon *DELPHI find_polygon_fast(Polygon *polygon, const Point *point)
{
    const Word32 x = point->x;
    const Word32 y = point->y;
    if (!positive_finite(x) || !positive_finite(y))
        return native_find_polygon(polygon, point);

    // Nonnegative finite binary32 numbers are ordered by their bit patterns.
    // Keep the original list order and exact native edge test for candidates.
    for (; polygon; polygon = polygon->next) {
        const Word32 left = polygon->left;
        const Word32 right = polygon->right;
        const Word32 top = polygon->top;
        const Word32 bottom = polygon->bottom;
        if ((x < left || x > right || y < top || y > bottom) &&
            positive_finite(left) && positive_finite(right) &&
            positive_finite(top) && positive_finite(bottom))
            continue;
        if (native_contains_point(polygon, point))
            return polygon;
    }
    return 0;
}

int DELPHI assign_group_fast(Polygon *polygon, const Point *point, int group)
{
    Polygon *match = find_polygon_fast(polygon, point);
    if (!match)
        return 0;
    if (match->group == -1)
        match->group = group;
    return match->group == group;
}

int DELPHI count_faction_stars_fast(const Galaxy *galaxy, Byte faction)
{
    const StarList *stars = galaxy->stars;
    int result = 0;
    for (int i = 0; i < stars->count; ++i) {
        const Star *star = stars->items[i];
        if (star->faction == faction) {
            // A Delphi WideString is a BSTR. Its byte length precedes its data.
            const Byte *custom = star->custom_faction;
            if (!custom || *(const Word32 *)(custom - 4) == 0)
                ++result;
        }
    }
    return result;
}

_Static_assert(OFFSET(RankedRanger, experience) == 0x3a8, "ranger experience");

void DELPHI sort_rangers_fast(const RankedRanger **rangers, int count)
{
    // Replay the native strict exchanges in the same order. Defer writing
    // rangers[i] until the row ends; later comparisons never read that slot.
    for (int i = 0; i < count - 1; ++i) {
        const RankedRanger *current = rangers[i];
        int current_experience = current->experience;
        for (int j = i + 1; j < count; ++j) {
            const RankedRanger *next = rangers[j];
            const int next_experience = next->experience;
            if (current_experience < next_experience) {
                rangers[j] = current;
                current = next;
                current_experience = next_experience;
            }
        }
        rangers[i] = current;
    }
}

float faction_ratio_fast(void)
{
    const Galaxy *galaxy = *(const Galaxy **)0x81d2a0;
    if (!galaxy || !galaxy->stars)
        return 0;
    const StarList *stars = galaxy->stars;
    int coalition = 0;
    int pirates = 0;
    for (int i = 0; i < stars->count; ++i) {
        const Star *star = stars->items[i];
        const Byte faction = star->faction;
        if (faction == 0 || faction == 2) {
            const Byte *custom = star->custom_faction;
            if (!custom || *(const Word32 *)(custom - 4) == 0) {
                coalition += faction == 0;
                pirates += faction == 2;
            }
        }
    }
    const int denominator = pirates > 1 ? pirates - 1 : 1;
    // The original converts both integers with FILD and rounds to Single.
    volatile float ratio = (long double)coalition / (long double)denominator;
    return ratio;
}
