#ifndef FAST_PATHS_H
#define FAST_PATHS_H

typedef unsigned char Byte;
typedef unsigned int Word32;
#define DELPHI __attribute__((regparm(3)))

typedef struct {
    Word32 x;
    Word32 y;
} Point;

typedef struct Polygon {
    void *vmt;
    struct Polygon *next;
    void *previous;
    void *points;
    int group;
    Byte other[0x14];
    Word32 left;
    Word32 top;
    Word32 right;
    Word32 bottom;
} Polygon;

typedef struct {
    Byte prefix[0x42];
    Byte faction;
    Byte alignment;
    const void *custom_faction;
} Star;

typedef struct {
    void *vmt;
    Star **items;
    int count;
} StarList;

typedef struct {
    Byte prefix[0x2c];
    StarList *stars;
} Galaxy;

typedef Byte (DELPHI *ContainsPoint)(const Polygon *, const Point *);
typedef Polygon *(DELPHI *FindPolygon)(Polygon *, const Point *);
extern ContainsPoint native_contains_point;
extern FindPolygon native_find_polygon;

Polygon *DELPHI find_polygon_fast(Polygon *polygon, const Point *point);
int DELPHI assign_group_fast(Polygon *polygon, const Point *point, int group);
int DELPHI count_faction_stars_fast(const Galaxy *galaxy, Byte faction);
float faction_ratio_fast(void);

typedef struct {
    Byte prefix[0x3a8];
    int experience;
} RankedRanger;
void DELPHI sort_rangers_fast(const RankedRanger **rangers, int count);

#endif
