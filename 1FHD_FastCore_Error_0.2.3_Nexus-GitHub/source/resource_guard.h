#ifndef RESOURCE_GUARD_H
#define RESOURCE_GUARD_H
#include "fast_paths.h"

typedef __WCHAR_TYPE__ Wide;
typedef void (__attribute__((stdcall)) *VisualLog)(const Wide *, const Wide *);
void set_visual_log(VisualLog log);
void DELPHI guarded_item_icon(const Wide *path, void *destination, int item_type);
void DELPHI guarded_portrait(void *image, const Wide *path);
void DELPHI guarded_animation(void *image, const Wide *path);

#endif
