#ifndef RESOURCE_GUARD_H
#define RESOURCE_GUARD_H
#include "fast_paths.h"

typedef __WCHAR_TYPE__ Wide;
typedef void (__attribute__((stdcall)) *VisualLog)(const Wide *, const Wide *);
const Wide *const *find_config_parameter(void *block, const Wide *key);
void set_visual_log(VisualLog log);
void DELPHI guarded_item_icon(const Wide *path, void *destination, int item_type);
void DELPHI guarded_item_image(void *image, const Wide *path);
void DELPHI guarded_item_animation(void *image, const Wide *path);
void DELPHI guarded_item_cursor(void *loop, const Wide *path, const void *hotspot);
void DELPHI guarded_award_icon(const Wide *key, void *destination);
void DELPHI guarded_award_path(void *screen, int id, const Wide **result);
void DELPHI guarded_award_text(const Wide *key, const Wide **result);
void DELPHI guarded_station_background(const Wide **result);
void DELPHI guarded_station_image(void *image, const Wide *path);
void hide_bridge_panels_entry(void);
void bridge_panel_entry(void);
void DELPHI guarded_portrait(void *image, const Wide *path);
void DELPHI guarded_animation(void *image, const Wide *path);
void DELPHI guarded_load_saved_message(void *entry, void *buffer);

#endif
