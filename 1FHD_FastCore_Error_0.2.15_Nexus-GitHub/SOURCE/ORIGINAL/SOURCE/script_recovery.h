#ifndef SCRIPT_RECOVERY_H
#define SCRIPT_RECOVERY_H

#include "resource_guard.h"

void set_script_recovery_log(VisualLog log);
struct SaveBuffer;
unsigned short DELPHI start_script_recovery(struct SaveBuffer *buffer);
void finish_script_recovery_entry(void);
void missing_script_entry(void);
void DELPHI guarded_script_turn_code(const void *code, void *process);

#endif
