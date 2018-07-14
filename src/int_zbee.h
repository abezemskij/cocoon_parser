// ZigBee inteface functions header
#include "definitions.h"
#include "formatting.h"
#include "fileutils.h"
#include "epoch_conv.h"
#include "validations.h"

#ifndef _INT_ZBEE_
void ieee_man_frames_handler(char *after_length_ptr, ZigBee_Frame *zb_object, Enum_Type* Enumerator);
void zigb_man_frames_handler(char *after_length_ptr, ZigBee_Frame *zb_object, Enum_Type* Enumerator);
unsigned int exctract_source_id(char *line, ZigBee_Frame *zb_object, Enum_Type *Enumerator);
ZigBee_Frame *convert_line_to_zb_header(ZigBee_Frame *frame, char *line);
ZigBee_Frame **process_zigbee_lines(char *ptr, unsigned long lines, unsigned int *filtered, Enum_Type *Enumerator);
void process_zigbee_file_input_live(unsigned char live_mode, char *line_buffer, char *out_filename_ptr, unsigned short argument_flags, char *args, Enum_Type *Enumerator);
#define _INT_ZBEE_
#endif
