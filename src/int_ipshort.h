// Interface IP Shortened version  functions header file
#include "definitions.h"
#include "formatting.h"
#include "fileutils.h"
#include "epoch_conv.h"
#include "validations.h"

#ifndef _INT_IPSHORT_
void process_ip_frame(char *line, IP_Frame *ip_frm, Enum_Type *Enumerator_Addr, Enum_Type *Enumerator_Proto);
IP_Frame **process_ip_frame_lines(char *ptr, unsigned long lines, unsigned int *filtered, Enum_Type *Enumerator_Addr, Enum_Type *Enumerator_Proto);
void process_ip_short_input_live(unsigned char live_mode, char *line_buffer, char *out_filename_ptr, unsigned short argument_flags, char *args, Enum_Type *Enumerator_Addr, Enum_Type *Enumerator_Proto);
#define _INT_IPSHORT_
#endif

