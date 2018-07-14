// Processor functions header file
#ifndef _ANALYSIS_PROCESS_

#include "windowing.h"
#include "math_func.h"

void pro_audio(SLOT *slot);
void pro_zbee(SLOT *slot, GLOBAL_KNOWLEDGE *glob);
void pro_wifi(SLOT *slot, GLOBAL_KNOWLEDGE *glob);
void pro_ip_short(SLOT *slot, GLOBAL_KNOWLEDGE *glob);
void process_slot(SLOT *slot, GLOBAL_KNOWLEDGE *glob, unsigned char type);

#define _ANALYSIS_PROCESS_
#endif
