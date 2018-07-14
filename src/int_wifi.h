// Interface Wifi 802.11 functions header file
#include "definitions.h"
#include "formatting.h"
#include "fileutils.h"
#include "epoch_conv.h"
#include "validations.h"

#ifndef _INT_WIFI_
void process_wifi_frame(char *line, WiFi_Frame *wifi_frm, Enum_Type *Enumerator_Addr, Enum_Type *Enumerator_Proto);
WiFi_Frame **process_wifi_lines(char *ptr, unsigned long lines, unsigned int *filtered, Enum_Type *Enumerator_Addr, Enum_Type *Enumerator_Proto);
void process_wifi_file_input_live(unsigned char live_mode, char *line_buffer, char *out_filename_ptr, unsigned short argument_flags, char *args, Enum_Type *Enumerator_Addr, Enum_Type *Enumerator_Proto);
#define _INT_WIFI_
#endif
