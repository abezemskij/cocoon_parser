
#include "argread.h"
#include "int_audio.h"
#include "int_ipshort.h"
#include "int_wifi.h"
#include "int_zbee.h"

#define FILENAME_BUFFER 128

char *descriptor_filename_start;

struct ZigBee_Frame Global_ZB_Pkt;
struct Enum_Type Enum_Start;
struct Enum_Type WiFi_Address;
struct Enum_Type IP_Address;
struct Enum_Type Proto_Address;

unsigned int Enum_Index = 0;
unsigned char live_descriptor_write = 1;
unsigned int Version = 1;

unsigned long line_offset_increments = 1;

unsigned long mlk_alloc = 0;
unsigned long mlk_free = 0;
unsigned char eapol_flag = 0;

void showHelpMessage(){
	printf("Usage: C_Parser -[io] (filename) -[lwzhsa] \n");
	printf(" -i\tInput file parameter followed by the filename \n");
	printf(" -o\tOutput file parameter followed by the filename \n");
	printf(" -l\tLive mode (pipe with tshark output)\n");
	printf(" -z\tZigBee input\n");
	printf(" -w\tWifi input\n");
	printf(" -s\tIP Short input\n");
	printf(" -a\tAudio Input\n");
	printf(" -h\tThis help menu\n");
}


int main(int argc, char *argv[])
{
	//printf("Hello World, %s\n", argv[0]);
	char *path = extract_path(argv[0]);
	Enum_Type *Enumerator_Addr = (Enum_Type *)calloc(1, sizeof(Enum_Type));
	Enum_Type *Enumerator_Proto = (Enum_Type *)calloc(1, sizeof(Enum_Type));
//	enum_init(&Enum_Start);
//	enum_init(&WiFi_Address);
//	enum_init(&IP_Address);
//	enum_init(&Proto_Address);
	// Initialization of required variables
	uint64_t *line_offsets = 0;
	uint64_t line_offset_index = 0;
	char *in_filename_ptr = (char*)calloc(FILENAME_BUFFER, sizeof(char));
	char *out_filename_ptr = (char*)calloc(FILENAME_BUFFER, sizeof(char));

	// process arguments
	unsigned short argument_flags = 0;
	argument_flags = argument_flagger(argc, argv, argument_flags, in_filename_ptr, out_filename_ptr);
	//load_maps();
	load_maps(argument_flags, Enumerator_Addr, argv[0]);
	if ((argument_flags & STDOUT_FL) == STDOUT_FL){
		in_filename_ptr = (char*) calloc(1, sizeof(char));
		out_filename_ptr= (char*) calloc(1, sizeof(char));
	}
	if (((argument_flags & HELP_FLAG) != 0)){ showHelpMessage(); return 0; }
	if (((argument_flags & ZIGB_FLAG) != 0) &&	// if zigbee flag & input flag & output flag
		((argument_flags & IN_F_FLAG) != 0) && 
		((argument_flags & OU_F_FLAG) != 0)){
			FILE *in_file = open_file(in_filename_ptr, "r", argv[0]);
			if (in_file != NULL){
				// Initial line offsets 100k
				line_offsets = (uint64_t*)calloc(LINE_INIT+1, sizeof(uint64_t));
				*line_offsets = 0; // first starts at offset zero
				line_offset_index++;
				uint64_t lines = get_number_of_lines(in_file, line_offsets, line_offset_index);
				fseek(in_file, 0, SEEK_SET);
				//char *test = extract_n_lines(in_file, line_p_session, 0);
//				process_zigbee_file_input(in_file, lines);
				//process_zigbee_file_input_live(LIVE_FLAG, line_buffer);
				printf("Was able to open file!\nAllocs: %lu Freed: %lu\n", mlk_alloc, mlk_free);
				fclose(in_file);
			}

	} else if (((argument_flags & LIVE_FLAG) != 0) &&	// if live flag & (output or stdout) & not(zigbee or wifi)
				(((argument_flags & OU_F_FLAG) != 0) || ((argument_flags & STDOUT_FL) != 0)) &&
				(((argument_flags & ZIGB_FLAG) == 0) || ((argument_flags & WIFI_FLAG) == 0))){

				char *line_buffer = (char*)calloc(1024, sizeof(char));
				unsigned int line_count = 0;
				if ((argument_flags & STDOUT_FL) != STDOUT_FL){
					FILE *out_file = open_file(out_filename_ptr, "w+", path);
					fwrite(&Version, sizeof(int), 1, out_file);
					fwrite(&line_count, sizeof(int), 1, out_file);
					fclose(out_file);
				}
				while(fgets(line_buffer, 1024, stdin) != NULL){
					if (((argument_flags & ZIGB_FLAG) == ZIGB_FLAG)){
						//printf("\nEn - %s\n", line_buffer);
						process_zigbee_file_input_live(LIVE_FLAG, line_buffer, out_filename_ptr, argument_flags, argv[0], Enumerator_Addr);

					} else if (((argument_flags & WIFI_FLAG) == WIFI_FLAG)){
						process_wifi_file_input_live(LIVE_FLAG, line_buffer, out_filename_ptr, argument_flags, argv[0], Enumerator_Addr, Enumerator_Proto);
					} else if (((argument_flags & IP_SHOR_F) == IP_SHOR_F)){
						process_ip_short_input_live(LIVE_FLAG, line_buffer, out_filename_ptr, argument_flags, argv[0], Enumerator_Addr, Enumerator_Proto);
					} else if (((argument_flags & AUDIO_FLA) == AUDIO_FLA)){
//						printf("\n%d", sizeof(Audio_Frame));
						process_audio_input_live(LIVE_FLAG, line_buffer, argv[0], argument_flags, out_filename_ptr);
					}
					line_count++;
				}

	} else if (((argument_flags & WIFI_FLAG) != 0) &&     // if wifi flag & input & output
                	((argument_flags & IN_F_FLAG) != 0) &&
                	((argument_flags & OU_F_FLAG) != 0)){

			FILE *in_file = open_file(in_filename_ptr, "r", path);
                        if (in_file != NULL){
                                // Initial line offsets 100k
                                line_offsets = (uint64_t*)calloc(LINE_INIT+1, sizeof(uint64_t));
                                *line_offsets = 0; // first starts at offset zero
                                line_offset_index++;
                                uint64_t lines = get_number_of_lines(in_file, line_offsets, line_offset_index);
                                fseek(in_file, 0, SEEK_SET);
                                //char *test = extract_n_lines(in_file, line_p_session, 0);
//                                process_wifi_file_input(in_file, lines);
                                printf("Was able to open file!\nAllocs: %lu Freed: %lu\n", mlk_alloc, mlk_free);
                                fclose(in_file);
                        }
	} else {
		showHelpMessage();
	}
//	cin.get();
	return 0;
}

