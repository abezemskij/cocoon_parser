// Audio inteface functions header
#include "definitions.h"

#ifndef _INT_AUDIO_
Audio_Frame **process_audio_frame_lines(char *ptr, unsigned long lines, unsigned int *filtered);
void process_audio_input_live(unsigned char live_mode, char *line_buffer, char *args, unsigned short argument_flags, char *out_filename_ptr);
#define _INT_AUDIO_
#endif

