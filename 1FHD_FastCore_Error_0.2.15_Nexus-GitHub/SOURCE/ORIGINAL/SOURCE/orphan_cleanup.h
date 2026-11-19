#ifndef ORPHAN_CLEANUP_H
#define ORPHAN_CLEANUP_H

#include "resource_guard.h"

void set_orphan_cleanup(int mode, VisualLog log);
void cleanup_saved_world_entry(void);
void cleanup_saved_image_entry(void);
void cleanup_combat_shot_entry(void);
void cleanup_missiles_open_scene_entry(void);
void cleanup_missiles_refresh_scene_entry(void);
void countable_unit_size_entry(void);
void countable_merge_weight_entry(void);
void guarded_countable_pickup(const Byte *frame);

#endif
