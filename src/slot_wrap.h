// slot wrapper function header file
#ifndef _SLOT_WRAP_
#include "./analysis/windowing.h"
#include "int_wifi.h"
#include "./analysis/math_func.h"
#include "./analysis/generic.h"

GLOBAL_KNOWLEDGE *perform_global_features(SLOT *slot, unsigned char type);
void free_slot_frame_type(SLOT *slot, unsigned char type);
void analyse_slot_add(SLOT *, void *, unsigned char,  unsigned char);

#define _SLOT_WRAP_
#endif
