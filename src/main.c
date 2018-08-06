#include "signal.h"
#include "argread.h"
#include "int_audio.h"
#include "int_ipshort.h"
#include "int_wifi.h"
#include "int_zbee.h"
#include "int_spectrum.h"
#include "slot_wrap.h"

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

unsigned char process_flag = 5;
unsigned char window_seconds = 0;

Enum_Type *Enumerator_Addr = (Enum_Type *)calloc(1, sizeof(Enum_Type));
Enum_Type *Enumerator_Proto = (Enum_Type *)calloc(1, sizeof(Enum_Type));
SLOT *slot = slot_init();
unsigned short argument_flags = 0;



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

void middleware_handler(char *line_buffer){
	
}
unsigned int identify_arg(){
	if ((argument_flags & ZIGB_FLAG) == ZIGB_FLAG) return 1;
	if ((argument_flags & WIFI_FLAG) == WIFI_FLAG) return 2;
	if ((argument_flags & AUDIO_FLA) == AUDIO_FLA) return 3;
	if ((argument_flags & SPECT_FLA) == SPECT_FLA) return 4;
	if ((argument_flags & IP_SHOR_F) == IP_SHOR_F) return 5;
	return 0;
}
void int_sigalarm(int sig){
	//printf("\n!!! INTERRUPT !!!\n");
	process_flag = 1;
	if (slot->n < 1){
		switch(identify_arg()){
			case 1:	// wifi
				printf("%" PRIu64 ",0,0,0,0,0,0,0,0,0,0,0,0,0\n", slot->slot_stop_time);
				break;
			case 2:	// ip
				printf("%" PRIu64 ",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n", slot->slot_stop_time);
				break;
			case 3:
				printf("%" PRIu64 ",0,0,0,0,0,0,0,0,0,0,0,0\n", slot->slot_stop_time); // possibly needs to move in to an if statement
				break;
			case 4: // spect
				printf("%" PRIu64 ",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n", slot->slot_stop_time);
				break;
			case 5:
				printf("%" PRIu64 ",0,0,0,0,0\n", slot->slot_stop_time);
				break;
			default:
				break;
		}
		slot->slot_stop_time = slot->slot_start_time+(1000000*window_seconds); slot->slot_start_time = slot->slot_stop_time; 
	}
	alarm(window_seconds);
}

int main(int argc, char *argv[])
{
	//printf("Hello World, %s\n", argv[0]);
	char *path = extract_path(argv[0]);
	usleep(1000000); // delay to allow to initially
	//Enum_Type *Enumerator_Addr = (Enum_Type *)calloc(1, sizeof(Enum_Type));
	//Enum_Type *Enumerator_Proto = (Enum_Type *)calloc(1, sizeof(Enum_Type));
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
	
	argument_flags = argument_flagger(argc, argv, argument_flags, in_filename_ptr, out_filename_ptr, &window_seconds);
	//load_maps();
	load_maps(argument_flags, Enumerator_Addr, argv[0]);
	unsigned char active_slot = 0;
	
	if (window_seconds != 0 ){
		signal(SIGALRM, int_sigalarm);
		alarm(window_seconds); process_flag = 4;
	} else {
		window_seconds = 1;
	}
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
				//process_zigbee_file_input(in_file, lines);
				//process_zigbee_file_input_live(LIVE_FLAG, line_buffer, out_filename_ptr, argument_flags, argv, Enumerator_Addr);
				printf("Was able to open file!\nAllocs: %lu Freed: %lu\n", mlk_alloc, mlk_free);
				fclose(in_file);
			}

	} else if (
	((argument_flags & LIVE_FLAG) != 0) &&	// if live flag & (output or stdout) & not(zigbee or wifi)
				(((argument_flags & OU_F_FLAG) != 0)
				 || ((argument_flags & STDOUT_FL) != 0)) &&
				(((argument_flags & ZIGB_FLAG) == 0) || ((argument_flags & WIFI_FLAG) == 0)) || ((argument_flags & SPECT_FLA) == SPECT_FLA)
				){

				char *line_buffer = (char*)calloc(1024, sizeof(char));
				unsigned int line_count = 0;
				if ((argument_flags & STDOUT_FL) != STDOUT_FL){
					FILE *out_file = open_file(out_filename_ptr, "w+", path);
					fwrite(&Version, sizeof(int), 1, out_file);
					fwrite(&line_count, sizeof(int), 1, out_file);
					fclose(out_file);
				}
				while((fgets(line_buffer, 1024, stdin) != NULL)){
					if (((argument_flags & ZIGB_FLAG) == ZIGB_FLAG)){
						//printf("\nEn - %s\n", line_buffer);
						zbee_struct_internal *test_zbee = (zbee_struct_internal*)calloc(1, sizeof(zbee_struct_internal));
						pro_zbee_int(line_buffer, test_zbee, Enumerator_Addr);
						analyse_slot_add(slot, (void*)test_zbee, sizeof(zbee_struct_internal), 3, Enumerator_Addr, &process_flag, window_seconds);
						if (process_flag == 2){ process_flag = 0; }
						//process_zigbee_file_input_live(LIVE_FLAG, line_buffer, out_filename_ptr, argument_flags, argv[0], Enumerator_Addr);

					} else if (((argument_flags & WIFI_FLAG) == WIFI_FLAG)){
						//process_wifi_file_input_live(LIVE_FLAG, line_buffer, out_filename_ptr, argument_flags, argv[0], Enumerator_Addr, Enumerator_Proto);
							wifi_struct_internal *test_wifi = (wifi_struct_internal*)calloc(1, sizeof(wifi_struct_internal));
							pro_wifi_int(line_buffer, test_wifi, Enumerator_Addr);
							analyse_slot_add(slot, (void*)test_wifi, sizeof(wifi_struct_internal), 1, Enumerator_Addr, &process_flag, window_seconds);
							if (process_flag == 2){ process_flag = 0; }
					} else if (((argument_flags & IP_SHOR_F) == IP_SHOR_F)){
						ip_struct_internal *test_ip = (ip_struct_internal*)calloc(1,sizeof(ip_struct_internal));
						pro_short_int(line_buffer, test_ip, Enumerator_Addr);
						analyse_slot_add(slot, (void*)test_ip, sizeof(ip_struct_internal), 2, Enumerator_Addr, &process_flag, window_seconds);
						if (process_flag == 2){ process_flag = 0;  }
						//process_ip_short_input_live(LIVE_FLAG, line_buffer, out_filename_ptr, argument_flags, argv[0], Enumerator_Addr, Enumerator_Proto);
					} else if (((argument_flags & AUDIO_FLA) == AUDIO_FLA)){
//						printf("\n%d", sizeof(Audio_Frame));
						audio_struct_internal *test_aud = (audio_struct_internal*)calloc(1,sizeof(audio_struct_internal));
						pro_audio_int(line_buffer, test_aud);
						analyse_slot_add(slot, (void*)test_aud, sizeof(audio_struct_internal), 5, Enumerator_Addr, &process_flag, window_seconds);
						if (process_flag == 2){ process_flag = 0; }
						//process_audio_input_live(LIVE_FLAG, line_buffer, argv[0], argument_flags, out_filename_ptr);
						//if (process_flag == 2){ process_flag = 0; alarm(window_seconds); }
					} else if (((argument_flags & SPECT_FLA) == SPECT_FLA)){
						spec_struct_internal *test_spec = (spec_struct_internal*)calloc(1,sizeof(spec_struct_internal));
						process_rf_output(line_buffer, test_spec);
						if (test_spec->n != 0) analyse_slot_add(slot, (void*)test_spec, sizeof(spec_struct_internal), 4, Enumerator_Addr, &process_flag, window_seconds);
						// process_spectrum_input
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

