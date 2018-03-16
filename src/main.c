// Cocoon_Parser.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fstream>


#define LIVE_FLAG  1<<0	// Live feed
#define IN_F_FLAG  1<<1	// Input filename parameter
#define OU_F_FLAG  1<<2	// Out filename parameter
#define WIFI_FLAG  1<<3	// Wifi mode
#define ZIGB_FLAG  1<<4	// Zigbee mode
#define AMODEFLAG  1<<5	// Auto mode, identify whether ZigBee or Wifi
#define HELP_FLAG  1<<6 // Help	
#define PARAMETERF 1<<7 // Parameter for input/output is parsed next
#define IP_SHOR_F  1<<8 // IP Short version
#define AUDIO_FLA  1<<9 // Audio flag in a format of <timestamp>,<value>
#define WIN_MODE 1
#define LINE_INIT  100000	// Lines initialized

using namespace std;


// Epoch time start offset
	const unsigned char month_epoch = 1, day_epoch = 1, hour_poch = 0, minute_poch = 0;
	const int year_epoch = 1970;
	const double sec_ms_epoch = 0;
	unsigned char leap_years =0;
	const unsigned long month_s[] = {2678400, 5097600, 7776000,10368000,13046400,15638400,18316800,20995200,23587200,26265600,28857600,31536000};
// END

typedef struct ZigBee_Frame{
	uint64_t timestamp;
	unsigned short src_id;
	unsigned short dst_id;
	unsigned char frame_type;
	unsigned char packet_size;
	unsigned char flags; // 1 Bad_Checksum
}ZigBee_Frame;
typedef struct IP_Frame{
	uint64_t timestamp;
	unsigned short src_ip;
	unsigned short dst_ip; // Potentially can be switched to short, analysis is required.
	unsigned short protocol;
	unsigned short packet_size;

}IP_Frame;
typedef struct Audio_Frame{
	uint64_t timestamp;
	double db;
}Audio_Frame;

typedef struct WiFi_Frame{
	uint64_t timestamp;
	unsigned short src_id;
	unsigned short dst_id;
	unsigned char  bssid_len;
	char*	bssid;
	unsigned char essid_len;
	char*	essid;
	unsigned short frame_type;
	unsigned short frame_length;
	unsigned short frame_sn;
	unsigned char  frame_fn;
//	unsigned short frame_flags;
}WiFi_Frame;

typedef struct Timestamp{
	unsigned long timestamp_s;
	unsigned long timestamp_ms;
}Timestamp;

typedef struct Enum_Type{
	unsigned short frame_type;
	char *name;
	struct Enum_Type *next;
}Enum_Type;

unsigned short argument_flags = 0;
char *in_filename_ptr, *out_filename_ptr;
char *path;
char *descriptor_filename_start;

struct ZigBee_Frame Global_ZB_Pkt;
struct Enum_Type Enum_Start;
struct Enum_Type WiFi_Address;
struct Enum_Type IP_Address;
struct Enum_Type Proto_Address;

unsigned int Enum_Index = 0;
unsigned char live_descriptor_write = 1;
unsigned int Version = 1;

uint64_t *line_offsets = 0;
uint64_t line_offset_index = 0;
unsigned long line_offset_increments = 1;

unsigned long mlk_alloc = 0;
unsigned long mlk_free = 0;
unsigned char eapol_flag = 0;



unsigned short extract_flag(char *arg){
	unsigned short args = 0;
	if(strcmp(arg, "-l") == 0){
		args |= LIVE_FLAG;
	} else if (strcmp(arg, "-i") == 0){
		args |= IN_F_FLAG;
		args |= PARAMETERF;
		in_filename_ptr = 0;
	} else if (strcmp(arg, "-o") == 0){
		args |= OU_F_FLAG;
		args |= PARAMETERF;
		out_filename_ptr = 0;
	} else if (strcmp(arg, "-w") == 0){
		args |= WIFI_FLAG;
	} else if (strcmp(arg, "-z") == 0){
		args |= ZIGB_FLAG;
	} else if (strcmp(arg, "-s") == 0){
		args |= IP_SHOR_F;
	} else if (strcmp(arg, "-a") == 0){
		args |= AUDIO_FLA;
	} else {
		if (argument_flags & PARAMETERF){
			if ((in_filename_ptr == 0) && ((argument_flags & IN_F_FLAG) != 0)){
				char *t = arg;
				unsigned char i = 0;
				while(*(t++) != 0)i++; // get length of the string
				in_filename_ptr = (char*)calloc(i+1, sizeof(char)); // clear the allocated memory
				mlk_alloc++;
				memcpy(in_filename_ptr, arg, i);
				argument_flags ^= PARAMETERF; return args;
			}
			if (out_filename_ptr == 0 && ((argument_flags & OU_F_FLAG) != 0)){
				char *t = arg;
				unsigned char i = 0;
				while(*(t++) != 0)i++; // get length of the string
				out_filename_ptr = (char*)calloc(i+1, sizeof(char)); // clear the allocated memory
				mlk_alloc++;
				memcpy(out_filename_ptr, arg, i);
				argument_flags ^= PARAMETERF; return args;
			}
		}
	}
	return args;
}

unsigned short argument_flagger(int argc, char* argv[]){
	unsigned char i = 1;
	if (argc > 1){
		while(--argc){
			argument_flags |= extract_flag(argv[i]);
			i++;
			if (argument_flags & HELP_FLAG) break; // if help is set then break the loop and show help
		}
	}
	return argument_flags;
}

unsigned char validate_flags(unsigned char args){
	unsigned char res = 0;
	// If live is set, then output is necessary
	if ((argument_flags & LIVE_FLAG)) if ((argument_flags & OU_F_FLAG)) res = 1;
	// If Input is set, then output is necessary
	if ((argument_flags & IN_F_FLAG)) if ((argument_flags & OU_F_FLAG)) res = 1;
	// If Auto, then no zigbee or wifi
	if ((argument_flags & AMODEFLAG)) if (!(argument_flags & ZIGB_FLAG) || !(argument_flags & WIFI_FLAG)) res = 1;
	// If Zigbee, then no wifi or auto
	if ((argument_flags & ZIGB_FLAG)) if (!(argument_flags & AMODEFLAG) || !(argument_flags & WIFI_FLAG)) res = 1;
	// If Wifi, then no auto or zigbee
	if ((argument_flags & WIFI_FLAG)) if (!(argument_flags & ZIGB_FLAG) || !(argument_flags & AMODEFLAG)) res = 1;
	// if IP Short, then no auto or zigbee or wifi
	if ((argument_flags & IP_SHOR_F)) if (!(argument_flags & ZIGB_FLAG) || !(argument_flags & WIFI_FLAG) || !(argument_flags & AMODEFLAG)) res = 1;
	return res;
}

char *extract_path(char *arg){
	char *ptr = arg;
	unsigned int len = 0;
	unsigned char slashes = 0;
	// find slashes (windows/unix) length, if full path provided
	while(*(ptr++) != 0){
		if ((*ptr == '\\') || (*ptr == '/')){ slashes++; }
		len++;
		//printf("+");
	}
	if (slashes == 0){ ptr = (char *)calloc(1, sizeof(char)); *ptr = '\0'; return ptr; }
	// extract path only
	ptr = arg+len;
	while(*(ptr--) != '\\'){
		if (*ptr == '/'){ len--; break; } // possibly len--
		len--;
		//printf("-");
	}
	ptr = (char *)calloc(len+2, sizeof(char));
	mlk_alloc++;
	memcpy(ptr, arg, len+1);
	return ptr;
}

void enum_init(Enum_Type *Enum){
	Enum->frame_type = 0x0000;
	Enum->name = 0x00;
	Enum->next = 0x00;
}
unsigned short enum_add(char *name, Enum_Type *Enum){
	unsigned int i = 0;
	unsigned short frame_type_id = 0;
	if(Enum->frame_type == 0){
		// first
		char *ptr = name;
 		while(*ptr++ != '\0')i++;
		//ptr += name_length;
		Enum->name = (char*)calloc(i+1,sizeof(char));
		memcpy(Enum->name, name, sizeof(char)*i);
		Enum->frame_type = 1;
		frame_type_id = Enum->frame_type;
		Enum->next = (Enum_Type*)calloc(i,sizeof(Enum_Type));
	} else {
		// crawl to the top
		unsigned int free_frame_type = Enum->frame_type;
		Enum_Type *pkt_ptr = Enum->next;
		while(pkt_ptr->name != 0x00) {free_frame_type++; pkt_ptr = pkt_ptr->next;}
		char *ptr = name;
		while(*ptr++ != '\0')i++;
		//ptr += name_length;
		pkt_ptr->name = (char*)calloc(i+1,sizeof(char));
		memcpy(pkt_ptr->name, name, sizeof(char)*i);
		pkt_ptr->frame_type = free_frame_type+1;
		frame_type_id = pkt_ptr->frame_type;
		pkt_ptr->next = (Enum_Type*)calloc(i,sizeof(Enum_Type));
	} 
	live_descriptor_write = 1;
	return frame_type_id;
}
unsigned short enum_add_num(char *name, Enum_Type *Enum, unsigned short frame_id){
	unsigned int i = 0;
        unsigned short frame_type_id = 0;

        if(Enum->frame_type == 0){
                // first
		char *ptr = name;
		while(*ptr++ != '\0')i++;
		Enum->name = (char*)calloc(i+1,sizeof(char));
	        memcpy(Enum->name, name, sizeof(char)*i);
        	Enum->frame_type = frame_id;
	        Enum->next = (Enum_Type*)calloc(i, sizeof(Enum_Type));
        } else {
                // crawl to the top
                unsigned int free_frame_type = Enum->frame_type;
                Enum_Type *pkt_ptr = Enum->next;
                while(pkt_ptr->name != 0x00) {pkt_ptr = pkt_ptr->next;}
                char *ptr = name;
                while(*ptr++ != '\0')i++;
                //ptr += name_length;
                pkt_ptr->name = (char*)calloc(i+1,sizeof(char));
                memcpy(pkt_ptr->name, name, sizeof(char)*i);
                pkt_ptr->frame_type = frame_id;
                frame_type_id = pkt_ptr->frame_type;
                pkt_ptr->next = (Enum_Type*)calloc(i,sizeof(Enum_Type));
        }
//	printf("%s - %d", name, frame_id);
        live_descriptor_write = 1;
        return frame_type_id;
}

unsigned short enum_find_frame_type(char *name, Enum_Type *Enum){
	Enum_Type *pkt_ptr = Enum;
	unsigned short res = 0xFFFF;
	unsigned char length = 0;
	char *ptr = name;
	while(*ptr++ != '\0') length++;
	ptr = name;
	do{
		if (pkt_ptr->name != 0x00){
			if(strcmp(name, pkt_ptr->name) == 0){ res = pkt_ptr->frame_type; break;}
			if(pkt_ptr->name != 0x00) pkt_ptr = pkt_ptr->next;
		}
	}while(pkt_ptr->name != 0x00);
	return res;
}

char *concat_path_and_filename(char *filename){
	char *ptr = 0;
	int len_p = 0;
	int len_f = 0;
	ptr = path;
	while(*(ptr++) != 0) len_p++;
	ptr = filename;
	while(*(ptr++) != 0) len_f++;
	ptr = (char*)calloc(len_p+len_f+1, sizeof(char));
	memcpy(ptr, path, len_p);
	memcpy(ptr+len_p, filename, len_f);
	return ptr;
	// free path
}
unsigned char process_zigbee_file(){
	unsigned char res = 0;

	return res;
}

unsigned char process_wifi_file(){
	unsigned char res = 0;

	return res;
}

unsigned char process_pipe(){
	unsigned char res = 0;
	return res;
}

FILE *open_file(char *filename, const char *mode){
	FILE *t = 0;
	char *full_path = concat_path_and_filename(filename);
	t = fopen(full_path, mode);
	free(full_path);
	return t;
}

unsigned char validate_line_zigbee(char *line){
	if(line == 0x00) return 0x00;
	while(*line++ != '\0'){
		if (*line == 'I'){
			if (*(line+1) == 'E'){
				if (*(line+2) == 'E') return 1; // IEEE
			}
		}
		if (*line == 'Z'){
			if (*(line+1) == 'i'){
				if (*(line+2) == 'g') return 1; // IEEE
			}
		}
	}
	return 0x00;
}
unsigned char validate_line_wifi(char *line){
	if (line == 0x00) return 0x00;
	while(*line++ != '\0'){
		if (*line == '8'){
			if(*(line+1) == '0'){
				if (*(line+2) == '2'){
					if(*(line+3) == '.'){
						if(*(line+4) == '1'){
							if(*(line+5) == '1'){
								return 1;
							}
						}
					}
				}
			}
		}
	}
	return 0x00;
}
unsigned char validate_line_wifi_eapol(char *line){
	if (line == 0x00) return 0x00;
	while(*line++ != '\0'){
		if (*line == 'E'){
			if (*(line+1) == 'A'){
				if (*(line+2) == 'P'){
					if (*(line+3) == 'O'){
						if(*(line+4) == 'L'){
							eapol_flag = 1;
							return 1;
						}
					}
				}
			}
		}
	}
	return 0;
}
uint64_t get_number_of_lines(FILE *file){
	uint64_t res = 0;
	int i = 0;
	int byte_read = 0;
	uint64_t pos = 0;
	char buff[4096];
	line_offset_increments = LINE_INIT;
	while((byte_read = fread(buff, sizeof(char), sizeof(buff), file)) > 0){
		while(i < byte_read){
			if (buff[i] == '\n'){
				*(line_offsets+line_offset_index) = pos+1; // compensate '\n'
				res++;
				line_offset_index++;
				if (line_offset_index == (line_offset_increments)){ // more than initial, allocate double, copy current, free old pointer
					uint64_t *new_ptr = (uint64_t*)calloc(((line_offset_increments*2)), sizeof(uint64_t));
					memcpy(new_ptr, line_offsets, sizeof(uint64_t)*line_offset_index);
					free(line_offsets);
					line_offsets = new_ptr;
					line_offset_increments *=2;
				}
			}
			pos++;
			i++;
		}
		i=0;
	}
	// clear out unneccessary memory
	if ((res) < (line_offset_increments*2)){
		uint64_t *new_ptr = (uint64_t*)calloc(line_offset_index, sizeof(uint64_t));
		//memcpy(new_ptr, line_offsets, sizeof(uint64_t)*line_offset_index);
		memcpy(new_ptr, line_offsets, sizeof(uint64_t)*line_offset_index);
		free(line_offsets);
		line_offsets = new_ptr;	
	}
	/*while(_t != EOF){
		_t = (char)fgetc(file);
		if (_t == '\n') res++;
	}*/
	return res;
}

uint64_t convert_date_to_epoch(char* line){ // requires trimmed version
	char *debug_line = line;
	uint64_t  ret_res = 0;
	char* year = (char*)calloc(5, sizeof(char)); // format 2017 +1 for 0x00
	char* month = (char*)calloc(3, sizeof(char)); // format 12 +1 for 0x00
	char* day = (char*)calloc(3, sizeof(char)); // format 27 +1 for 0x00
	char* hour= (char*)calloc(3, sizeof(char));
	char* minute= (char*)calloc(3, sizeof(char));
	char* sec_ms= (char*)calloc(13, sizeof(char));

	unsigned char month_i, day_i, hour_i, minute_i;
	int year_i = 0;
	double sec_ms_i = 0;
	// 1509006715
	// find first space
	while(*line != ' ') line++; // ignore the padding spaces if they exist and the number
	while(*line == ' ') line++;
	memcpy(year, line, 4); // format 2017
	while(*line++ != '-');
	memcpy(month, line, 2);
	while(*line++ != '-');
	memcpy(day, line, 2);
	while(*line++ != ' ');
	memcpy(hour, line, 2);
	while(*line++ != ':');
	memcpy(minute, line, 2);
	while(*line++ != ':');
	memcpy(sec_ms, line, 12);

	// conversion
	year_i = atoi(year)		-year_epoch;	// this may differ, compensate for 0th year
	month_i = atoi(month)	-month_epoch;	// this may differ
	day_i = atoi(day)		-day_epoch;		// this may differ
	hour_i = atoi(hour);				// time offset is always 0, no need to subtract
	minute_i = atoi(minute);
	sec_ms_i = atof(sec_ms)*1000000.0;	// ns precision dd.mmmnnn
	ret_res += (365*year_i)*86400;
	ret_res += month_s[month_i-1];//2628000; 
	ret_res += (day_i)*86400;
	ret_res += hour_i*3600;
	ret_res += minute_i*60;

	if (leap_years == 0) leap_years= ++year_i/4;
	ret_res += (leap_years)*86400;
	ret_res *= 1000000;
	ret_res += (int)sec_ms_i;

	//capture year until - is found
	//capture month until - is found
	//capture day until ' ' is found

	free(year);
	free(month);
	free(day);
	free(hour);
	free(minute);
	free(sec_ms);

	return ret_res;
}

unsigned char manage_enumerations(Enum_Type *Enum, char *start_pointer, char* end_pointer){
	char *man_type = (char*)calloc((end_pointer-start_pointer)+1, sizeof(char));mlk_alloc++;
	memcpy(man_type, start_pointer, sizeof(char)*(end_pointer-start_pointer));
	unsigned short ft = enum_find_frame_type(man_type, &Enum_Start);
	if(ft == 0xFFFF){
		ft = enum_add(man_type, &Enum_Start);
	}
	free(man_type);
	mlk_free++;
	return ft;
}
unsigned char manage_comparison(char *start_pointer, char terminating_char, const char *value){
	char *t_ptr = start_pointer;
	unsigned char res = 0;
	while(*t_ptr++ != terminating_char){res++; if (*t_ptr == ','){start_pointer=t_ptr; res=0;}}
	char *man_type = (char*)calloc((t_ptr-start_pointer)+1, sizeof(char));
	memcpy(man_type, start_pointer, (t_ptr-start_pointer));
	if (strcmp(man_type, value) == 0){ free(man_type); return 1; }
	free(man_type);
	return 0;
}

void ieee_man_frames_handler(char *after_length_ptr, ZigBee_Frame *zb_object){
	char *t_ptr = after_length_ptr;
	while(*t_ptr != '\0'){
		if (*t_ptr++ == ','){
			t_ptr -=1; // compensate space
			// get the value from after_length_ptr to t_ptr
			zb_object->frame_type = manage_enumerations(&Enum_Start, after_length_ptr, t_ptr);
			// compensate ", "
			t_ptr +=2;
			//reusing all local variables
			if (!(zb_object->flags & 1))zb_object->flags |= manage_comparison(t_ptr, '\0', "Bad FCS");
			// quickly find Bad FCS
			if (zb_object->flags == 0){
				// test special cases
				while(*t_ptr++ != '\0'){
					//look for B
					if (*t_ptr == 'B'){
						if (*(t_ptr+1) == 'a'){
							// at this point it will be Ba(d FCS)
							zb_object->flags = 1;
						}
					}
				}
			}
			break;
		}
		if (*t_ptr == '\0'){
			zb_object->frame_type = manage_enumerations(&Enum_Start, after_length_ptr, t_ptr);;
			break;
		}
	}
}
void zigb_man_frames_handler(char *after_length_ptr, ZigBee_Frame *zb_object){
	char *t_ptr = after_length_ptr;
	while(*t_ptr != '\0'){
		if (*t_ptr++ == ','){
			t_ptr -=1; // compensate space
			// get the value from after_length_ptr to t_ptr
			zb_object->frame_type = manage_enumerations(&Enum_Start, after_length_ptr, t_ptr);
			// compensate ", "
			t_ptr +=2;
			//reusing all local variables
			if (!(zb_object->flags & 1))zb_object->flags |= manage_comparison(t_ptr, '\0', "Bad FCS");
			break;
		}
		if (*t_ptr == '\0'){
			zb_object->frame_type = manage_enumerations(&Enum_Start, after_length_ptr, t_ptr);
			break;
		}
	}
}
unsigned char load_maps(){
	FILE *address_map;
	FILE *protocol_map;
	
	char fname_adr[] = "address_map.csv";
	char fname_pro[] = "protocol_map.csv";
	char buffer[1024];
	unsigned short proto, addr;
	if ((access(fname_adr, F_OK) != -1) && (access(fname_pro, F_OK) != -1)){
		address_map = open_file(fname_adr, "r+");
		protocol_map= open_file(fname_pro, "r+");
		unsigned comma_index;
		while(fgets(buffer, sizeof(buffer), address_map)){
			comma_index = 0;
			char *t_ptr = (char*)buffer;
			while(*t_ptr != 0){	// get to comma
				if (*t_ptr == ','){ *t_ptr =0; t_ptr++; break;}
				comma_index++;
				t_ptr++;
			}
			addr = atoi(buffer);
			char *tt_ptr = t_ptr;
			while (t_ptr != 0){
				if (*t_ptr == '\n'){ *t_ptr =0; break;}
				t_ptr++;
			}
			if (argument_flags & IP_SHOR_F)enum_add_num(tt_ptr, &IP_Address, addr);
			if (argument_flags & WIFI_FLAG)enum_add_num(tt_ptr, &WiFi_Address, addr);
		}
		while(fgets(buffer, sizeof(buffer), protocol_map)){
			comma_index = 0;
                        char *t_ptr = (char*)buffer;
                        while(*t_ptr != 0){     // get to comma
                                if (*t_ptr == ','){ *t_ptr =0; t_ptr++; break;}
                                comma_index++;
                                t_ptr++;
                        }
                        addr = atoi(buffer);
                        char *tt_ptr = t_ptr;
                        while (t_ptr != 0){
                                if (*t_ptr == '\n'){ *t_ptr =0; break;}
                                t_ptr++;
                        }
                        if (argument_flags & IP_SHOR_F)enum_add_num(tt_ptr, &Proto_Address, addr);
			if (argument_flags & WIFI_FLAG)enum_add_num(tt_ptr, &Enum_Start, addr);
		}
	}
	return 0;
}
unsigned int exctract_source_id(char *line, ZigBee_Frame *zb_object){
	// use int to store src and dst
	unsigned short src = 0;
	unsigned short dst = 0;
	unsigned int srcdst= 0;
	char *t_ptr = line;

	while(*t_ptr++ != ' ');	// skip the number
	while(*t_ptr++ != ' '); // skip the date
	while(*t_ptr++ != ' '); // skip the time
	while(*t_ptr++ == ' '); // skip spaces
	// do the stuff, extract the source
	if (*t_ptr == (char)0x86 || *t_ptr == (char)0x3e){ // 0xe2 0x86 (arrow) (No addressess provided) (check if IEEE 802.15.4)
		t_ptr++;
		//0x86 compensate
		while(*++t_ptr == ' '); // go to next field IEEE, ZigBee
		unsigned short bcast = 0x0000;
		if (*t_ptr == 'B'){ bcast = 0xFFFF; while(*++t_ptr != ' '); while(*++t_ptr == ' ');}
		char ieee[5] = {0, 0, 0, 0, 0};
		char zigb[7] = {0, 0, 0, 0, 0, 0 ,0};
		char strd[9] = {0, 0, 0, 0, 0, 0 ,0, 0, 0};
		memcpy(ieee, t_ptr, sizeof(char)*4);
		if (strcmp(ieee, "IEEE") == 0){
			// IEEE
			while(*t_ptr++ != ' ');
			memcpy(strd, t_ptr, sizeof(char)*8);
			if (strcmp(strd, "802.15.4") == 0){
				// source
				srcdst = 0; // no addresses involved i.e. src and dst = 0;
				while(*t_ptr++ != ' '); //shift to text
				char *_diff = t_ptr;
				while(*_diff++ != ' ');
				unsigned char diff_v = (_diff-t_ptr);
				char *len = (char*)calloc(diff_v+1, sizeof(char));
				memcpy(len, t_ptr, sizeof(char)*diff_v);
				unsigned char ln = (char)atoi(len);
				t_ptr += diff_v;
				zb_object->src_id = 0x0000;
				zb_object->dst_id = 0x0000;
				if (bcast != 0) zb_object->dst_id = bcast;
				zb_object->packet_size = ln;
				// Ack Packet handler
				if ((zb_object->packet_size == 19) && (manage_comparison(t_ptr, '\0', "Ack") == 1)){
					zb_object->src_id = 0xFFFF;
	                                zb_object->dst_id = 0xFFFF;
				}
				if ((zb_object->src_id == 0x0000) && (zb_object->dst_id == 0x0000)){
					zb_object->flags = 1; // Reserved[Malformed Packet] case
				}
				ieee_man_frames_handler(t_ptr, zb_object);
				free(len);
				//extract the length
				//extract the frame type
			}
		} else {
			memcpy(zigb, t_ptr, sizeof(char)*6);
			if (strcmp(zigb, "ZigBee") == 0){
				// ZigBee
				srcdst = 0; // no addresses involved i.e. src and dst = 0;
				while(*t_ptr++ != ' '); //shift to text
				char *_diff = t_ptr;
				while(*_diff++ != ' ');
				unsigned char diff_v = (_diff-t_ptr);
				char *len = (char*)calloc((_diff-t_ptr)+1, sizeof(char));
				memcpy(len, t_ptr, sizeof(char)*(_diff-t_ptr));
				unsigned char ln = (char)atoi(len);
				t_ptr += diff_v;
				zb_object->src_id = src;
				zb_object->dst_id = dst;
				if (bcast != 0) zb_object->dst_id = bcast;
				zb_object->packet_size = ln;
				if ((zb_object->src_id == 0x0000) && (zb_object->dst_id == 0x0000)){
                                        zb_object->flags = 1; // Reserved[Malformed Packet] case
                                }
				zigb_man_frames_handler(t_ptr, zb_object);
				free(len);
			}
		}
	} else if (*t_ptr == 'x'){ // i.e. 0x
		t_ptr++;
		char *src_p = (char*)calloc(5, sizeof(char));
		char *dst_p = (char*)calloc(5, sizeof(char));
		memcpy(src_p, t_ptr, sizeof(char)*4);
		src = (short)strtol(src_p, NULL, 16);
		while(1){
			if (*t_ptr == 'x') break;
			if (*t_ptr == 'B') break;
			if (*t_ptr == 'Z'){t_ptr=t_ptr-3; *t_ptr='B'; t_ptr--; break; } //  17:51:57.945093901       0x0000 →              ZigBee 42 Beacon, S... case
			//if (*t_ptr == '\0') return;
			t_ptr++; // while not other address or Broadcast || (*t_ptr != 'B')
		}
		if((*t_ptr++) == 'x'){
			memcpy(dst_p, t_ptr, sizeof(char)*4);
			dst = (short)strtol(dst_p, NULL, 16);
		} else { // Broadcast
			dst = 0xFFFF;
		}
		srcdst = (src<<16) | dst;
		while(*++t_ptr != ' ');
		while(*++t_ptr == ' ');
		char ieee[5] = {0, 0, 0, 0, 0};
		char zigb[7] = {0, 0, 0, 0, 0, 0 ,0};
		char strd[9] = {0, 0, 0, 0, 0, 0 ,0, 0, 0};
		memcpy(ieee, t_ptr, sizeof(char)*4);
		if (strcmp(ieee, "IEEE") == 0){
			// IEEE
			while(*t_ptr++ != ' ');
			memcpy(strd, t_ptr, sizeof(char)*8);
			if (strcmp(strd, "802.15.4") == 0){
				// source
				srcdst = 0; // no addresses involved i.e. src and dst = 0;
				while(*t_ptr++ != ' '); //shift to text
				char *_diff = t_ptr;
				while(*_diff++ != ' ');
				unsigned char diff_v = (_diff-t_ptr);
				char *len = (char*)calloc((_diff-t_ptr)+1, sizeof(char));
				memcpy(len, t_ptr, sizeof(char)*(_diff-t_ptr));
				unsigned char ln = (char)atoi(len);
				t_ptr += diff_v;
				zb_object->src_id = src;
				zb_object->dst_id = dst;
				zb_object->packet_size = ln;
				if ((zb_object->src_id == 0x0000) && (zb_object->dst_id == 0x0000)){
                                        zb_object->flags = 1; // Reserved[Malformed Packet] case
                                }
				ieee_man_frames_handler(t_ptr, zb_object);
				free(len);
				//extract the length
				//extract the frame type
			}
		} else {
			memcpy(zigb, t_ptr, sizeof(char)*6);
			if (strcmp(zigb, "ZigBee") == 0){
				// ZigBee
				srcdst = 0; // no addresses involved i.e. src and dst = 0;
				while(*t_ptr++ != ' '); //shift to text
				char *_diff = t_ptr;
				while(*_diff++ != ' ');
				unsigned char diff_v = (_diff-t_ptr);
				char *len = (char*)calloc((_diff-t_ptr)+1, sizeof(char));
				memcpy(len, t_ptr, sizeof(char)*(_diff-t_ptr));
				unsigned char ln = (char)atoi(len);
				t_ptr += diff_v;
				zb_object->src_id = src;
				zb_object->dst_id = dst;
				zb_object->packet_size = ln;
				if ((zb_object->src_id == 0x0000) && (zb_object->dst_id == 0x0000)){
                                        zb_object->flags = 1; // Reserved[Malformed Packet] case
                                }
				zigb_man_frames_handler(t_ptr, zb_object);
				//printf("ZigBee!!! Length: %d\n", (int)ln);
				free(len);
			}
		}
		free(src_p);
		free(dst_p);
	}
	return srcdst;
}

ZigBee_Frame *convert_line_to_zb_header(ZigBee_Frame *frame, char *line){
	ZigBee_Frame *ret_ptr = (ZigBee_Frame*)calloc(1, sizeof(ZigBee_Frame));mlk_alloc++;
	ret_ptr->timestamp = convert_date_to_epoch(line);
	ret_ptr->src_id = 0x0000;
	ret_ptr->dst_id = 0x0000;
	ret_ptr->packet_size = 0;
	ret_ptr->frame_type = 0;
	ret_ptr->flags = 0;

	return ret_ptr;
}

char *extract_next_line(FILE *file, int *len){
	char *ret_ptr = 0;
	// allocate temp buffer
	char *buffer = (char*)calloc(1024, sizeof(char)); // max line length is 1024mlk_alloc++;
	char *buf_ptr = buffer;
	int i = 0;
	int cur_pos = 0;
	do{
		*buf_ptr++ = (char)fgetc(file);
		cur_pos++;
		if (*(buf_ptr-1) == '\n' || *(buf_ptr-1) == 0xFF){  cur_pos--; break; }
		if (*(buf_ptr-1) == EOF) { free(buffer); return 0x00; }
	}while(*(buf_ptr-1) != EOF);
	//trim spaces
	buf_ptr = buffer;
	while(*(buf_ptr+i++) == ' '); // point at not space char
	i--; buf_ptr += i; 

	ret_ptr = (char*)calloc((cur_pos-i+1), sizeof(char)); // allocate sentence space
	memcpy(ret_ptr, buf_ptr, (cur_pos-i));
	free(buffer);
	return ret_ptr;
}

void write_descriptor(Enum_Type *Enum, const char *file){
	char filename[64] = { 0 };
	int i = 0;
	while(*(file+i) != '\0')i++;
	int j = 0;
	while(*(out_filename_ptr+j) != '\0')j++;
	char *final = (char*)calloc(1, sizeof(char)*(i+j+2));
	memcpy(final, out_filename_ptr, j-4);
	memcpy((final+j-4), "_", 1);
	memcpy((final+j+1-4), file, i); 
	memcpy(filename, file, i);
	FILE *desc_file = open_file(final, "w+");
	Enum_Type *ptr = Enum;
	// loop through all Enums
	char buf[128];
	unsigned char str_len = 0;
	while(ptr->name != 0x00){
		//sprintf_s(buf, "%d,%s\n", ptr->frame_type, ptr->name);
		snprintf(buf, sizeof(buf), "%d,%s\n", ptr->frame_type, ptr->name);
		while(buf[str_len++] != '\0');
		str_len--;
		fwrite(buf, str_len, 1, desc_file);
		ptr = ptr->next;
		str_len = 0;
	}
	free(final);
	fclose(desc_file);
}
/*
void write_out_frames(void *Object, int num, char feedback_char, unsigned int overall_lines){
	printf("%c", feedback_char);
	int i = 0;
	if (((argument_flags & (OU_F_FLAG | ZIGB_FLAG)) == (OU_F_FLAG | ZIGB_FLAG))){
		ZigBee_Frame *frames = (ZigBee_Frame*)Object;
		int *arr_addr = (int*)Object;
		int *addr = (int*)*arr_addr;
		//unsigned int *addr = (int*)*arr_addr;
		int frame_size = sizeof(ZigBee_Frame);
		FILE *out_file = open_file(out_filename_ptr, "r+b");
		fseek(out_file, 0, SEEK_END);
		while(i < num){
			//ZigBee_Frame *frame_ptr = (ZigBee_Frame*)*frames;
			addr = (int*)(*(arr_addr+i++));
			fwrite(&frame_size, sizeof(int), 1, out_file);
			fwrite((void*)addr, sizeof(ZigBee_Frame)-1,1, out_file); // -1 because of structure padding
		}
		fseek(out_file, sizeof(int), SEEK_SET);
		fwrite(&overall_lines, sizeof(int), 1, out_file);
		fclose(out_file);
		i = 0;
		while(i < num){
			free((char*)(*(arr_addr+i++)));
		}
	} else if (((argument_flags & (OU_F_FLAG | WIFI_FLAG)) == (OU_F_FLAG | WIFI_FLAG))){
		// write out wifi frames
	}
}

char *copy_extract_n_lines(FILE *file, long n_lines, long *offset){
	char *ptr = 0;
	unsigned int line_count = 0;
	long char_pos = 0;
	int cur_line_len = 0;
	unsigned int zb_frame_len = sizeof(ZigBee_Frame);
	FILE *out_file = open_file(out_filename_ptr, "w+");
	fwrite(&Version, sizeof(int), 1, out_file);
	fwrite(&line_count, sizeof(int), 1, out_file);
	//fwrite(&zb_frame_len, sizeof(int), 1, out_file);
	fclose(out_file);
	ZigBee_Frame *zb_arr[1000];
	int zb_arr_i = 0;
	printf("Start\n");
	while(line_count != n_lines){
		zb_arr[zb_arr_i] = (ZigBee_Frame*)calloc(1, sizeof(ZigBee_Frame));
		ptr = extract_next_line(file, &cur_line_len);
		if (!validate_line_zigbee(ptr)){ if (ptr == 0x00){free(ptr);free(zb_arr[zb_arr_i]); break;}free(ptr); free(zb_arr[zb_arr_i]); continue;}
		zb_arr[zb_arr_i]->timestamp = convert_date_to_epoch(ptr);
		exctract_source_id(ptr, zb_arr[zb_arr_i]);
		free(ptr);
		zb_arr_i++;
		line_count++;
		if (zb_arr_i == 1000){
			write_out_frames((void*)zb_arr, zb_arr_i, '.', line_count);
			zb_arr_i = 0;
		}
		//free(ret_ptr);
	}
	write_out_frames((void*)zb_arr, zb_arr_i, '.', line_count);
	write_descriptor(&Enum_Start, "descriptor.csv");
	printf("\nOverall lines: %d, Processed: %d\n", n_lines, line_count);
	return ptr;
}
*/

char *extract_number_of_lines(FILE *file, long start_line_offset, long end_line_offset){
	start_line_offset = *(line_offsets+start_line_offset);
	end_line_offset = *(line_offsets+end_line_offset);
	long diff = (end_line_offset-start_line_offset);
	fseek(file, start_line_offset,SEEK_SET);
	char *ptr = (char*)calloc(diff+1, sizeof(char));
	fread(ptr, sizeof(char), diff, file);
	return ptr;
}
char *extract_line(char *lines, char **updated_offset){
	char *ptr = lines;
	int i = 0;
	int p = 0;
	if (*ptr == '\n'){ p++; ptr++; } // skip new line if exists 
	while(*ptr == ' '){ p++; ptr++; } //skip padding
	while(*ptr != '\n'){ i++; ptr++; }
	updated_offset[0] = ptr;
	ptr = (char*)calloc(i+1, sizeof(char));
	memcpy(ptr, lines+p, sizeof(char)*i);
	return ptr;
}

void write_out_frames_new(void *Object, int num, char feedback_char, unsigned int overall_lines){
	if (feedback_char != '\0')printf("%c", feedback_char);
	int i = 0;
	if (((argument_flags & (OU_F_FLAG | ZIGB_FLAG)) == (OU_F_FLAG | ZIGB_FLAG))){
		ZigBee_Frame **frames = (ZigBee_Frame**)Object;
		if (Object == 0x00){ free(Object); return;}
		//ZigBee_Frame *frames = (ZigBee_Frame*)Object;
		//int *arr_addr = (int*)Object;
		//int *addr = (int*)*arr_addr;
		//unsigned int *addr = (int*)*arr_addr;
		char frame_size = sizeof(ZigBee_Frame)-1; // -1 because of structure padding
		FILE *out_file = open_file(out_filename_ptr, "r+b");
		fseek(out_file, 0, SEEK_END);
		while(i < num){
			//ZigBee_Frame *frame_ptr = (ZigBee_Frame*)*frames;
			if( frames[i]->timestamp == 0) break;
			fwrite(&frame_size, sizeof(char), 1, out_file);
			fwrite(frames[i++], sizeof(ZigBee_Frame)-1,1, out_file); // -1 because of structure padding
		}
		fseek(out_file, sizeof(int), SEEK_SET);
		int write_processed = 0;
		fread(&write_processed, sizeof(int), 1, out_file);
		write_processed += i;
		fseek(out_file, sizeof(int), SEEK_SET);
		fwrite(&write_processed, sizeof(int), 1, out_file);
		fclose(out_file);
		i = 0;
		while(i < num){
			free(frames[i++]);
		}
	} else if (((argument_flags & (OU_F_FLAG | WIFI_FLAG)) == (OU_F_FLAG | WIFI_FLAG))){
		// write out wifi frames
		// free bssid if bssid Length is > 0
		WiFi_Frame **frames = (WiFi_Frame**)Object;
                FILE *out_file = open_file(out_filename_ptr, "r+b");
                fseek(out_file, 0, SEEK_END);
		// first we need to identify length of a dynamic frame
		// if 
                while(i < num){
			unsigned char obj_size = 8 + 2 + 2 + 1 + 1 + 1 + 2 + 2 +1; // time,src_id,dst_id,bssid_len,essid_len,frm_type,len,sn,fn
			if (frames[i]->bssid_len > 0){
				obj_size += frames[i]->bssid_len;
			}
			if (frames[i]->essid_len > 0){
				obj_size += frames[i]->essid_len;
			}
			fwrite(&obj_size, sizeof(char), 1, out_file);
                        if( frames[i]->timestamp == 0) break;
			fwrite(&frames[i]->timestamp, sizeof(char), 8, out_file);
			fwrite(&frames[i]->src_id, sizeof(char), 2, out_file); // short, avoiding x64 clash
			fwrite(&frames[i]->dst_id, sizeof(char), 2, out_file); // short, avoiding x64 clash
			fwrite(&frames[i]->bssid_len, sizeof(char), 1, out_file);
			//printf("\n%d\n", frames[i]->bssid_len);
			if (frames[i]->bssid_len > 0){ fwrite(frames[i]->bssid, sizeof(char), frames[i]->bssid_len, out_file); free(frames[i]->bssid); }
			fwrite(&frames[i]->essid_len, sizeof(char), 1, out_file);
                        if (frames[i]->essid_len > 0){ fwrite(frames[i]->essid, sizeof(char), frames[i]->essid_len, out_file); free(frames[i]->essid); }
			fwrite(&frames[i]->frame_type, sizeof(char), 1, out_file);
			fwrite(&frames[i]->frame_sn, sizeof(char), 2, out_file);
			fwrite(&frames[i]->frame_fn, sizeof(char), 1, out_file);
			fwrite(&frames[i]->frame_length, sizeof(char), 2, out_file);
			i++;
                }
                fseek(out_file, sizeof(int), SEEK_SET);
                int write_processed = 0;
                fread(&write_processed, sizeof(int), 1, out_file);
                write_processed += i;
                fseek(out_file, sizeof(int), SEEK_SET);
                fwrite(&write_processed, sizeof(int), 1, out_file);
                fclose(out_file);
                i = 0;
		while(i < num){
                        free(frames[i++]);
                }
	} else if (((argument_flags & (OU_F_FLAG | IP_SHOR_F)) == (OU_F_FLAG | IP_SHOR_F))){
		unsigned char obj_size = sizeof(IP_Frame);
		IP_Frame **frames = (IP_Frame**)Object;
		FILE *out_file = open_file(out_filename_ptr, "r+b");
		fseek(out_file, 0, SEEK_END);
		while(i < num){
			fwrite(&obj_size, sizeof(char), 1, out_file);
			if(frames[i]->timestamp == 0) break;
			fwrite(&frames[i]->timestamp, sizeof(char), 8, out_file);
                        fwrite(&frames[i]->src_ip, sizeof(short), 1, out_file); // short, avoiding x64 clash
                        fwrite(&frames[i]->dst_ip, sizeof(short), 1, out_file); // short, avoiding x64 clash
                        fwrite(&frames[i]->protocol, sizeof(short), 1, out_file);
			fwrite(&frames[i]->packet_size, sizeof(short), 1, out_file);
			i++;
		}
		fseek(out_file, sizeof(int), SEEK_SET);
                int write_processed = 0;
                fread(&write_processed, sizeof(int), 1, out_file);
                write_processed += i;
                fseek(out_file, sizeof(int), SEEK_SET);
                fwrite(&write_processed, sizeof(int), 1, out_file);
                fclose(out_file);
                i = 0;
                while(i < num){
                        free(frames[i++]);
                }
	} else if (((argument_flags &(OU_F_FLAG | AUDIO_FLA)) == (OU_F_FLAG | AUDIO_FLA))){
		unsigned char obj_size = sizeof(Audio_Frame);
                Audio_Frame **frames = (Audio_Frame**)Object;
                FILE *out_file = open_file(out_filename_ptr, "r+b");
                fseek(out_file, 0, SEEK_END);
                while(i < num){
                        fwrite(&obj_size, sizeof(char), 1, out_file);
                        if(frames[i]->timestamp == 0) break;
                        fwrite(&frames[i]->timestamp, sizeof(char), 8, out_file);
			fwrite(&frames[i]->db, sizeof(double), 1, out_file);
                        i++;
                }
                fseek(out_file, sizeof(int), SEEK_SET);
                int write_processed = 0;
                fread(&write_processed, sizeof(int), 1, out_file);
                write_processed += i;
                fseek(out_file, sizeof(int), SEEK_SET);
                fwrite(&write_processed, sizeof(int), 1, out_file);
                fclose(out_file);
                i = 0;
                while(i < num){
                        free(frames[i++]);
                }
	}
	free(Object);
}
char *return_me_not_char(char *ptr, char not_char){
	if (*ptr == not_char){
		while(*ptr++ == not_char);
		return (ptr-1);
	}
	return ptr;
}
void process_ip_frame(char *line, IP_Frame *ip_frm){
	// 31541 2017-11-24 15:17:40.826158 23.215.61.90 -> 192.168.1.3  HTTP 314
	char *t_ptr = line;
	// trim if starts from space
	if (*t_ptr == ' '){ while(*t_ptr++ == ' '); t_ptr--;} // trim first space and return to the first char
	while(*t_ptr++ != ' '); // skip the number, return value will be next space
	while(*t_ptr++ != ' '); // skip the number
	while(*t_ptr++ != ' '); // skip date
	if (*t_ptr == ' '){
		while(*t_ptr++ == ' ');
		t_ptr--;
	}

	if ((*t_ptr == (char)0x86) || (*t_ptr == (char)0x3e)){
		// no source found, capitulate!
	} else {
		char *src_ip = t_ptr;
		char *dst_ip = t_ptr;
		unsigned char i = 0;
		// validate that it is an IP, this should be validated prior
		while(*dst_ip++ != ' ')i++;
		t_ptr = return_me_not_char(t_ptr, ' ');

		char *source_ip = (char*)calloc(1, i+1);
		char *destin_ip = 0x00;
		memcpy(source_ip, src_ip, i);
		t_ptr += i+1;
		while(*t_ptr++ != ' '); // remove -> from text
		t_ptr = return_me_not_char(t_ptr, ' ');
		i = 0;
		src_ip = t_ptr;
		dst_ip = t_ptr;
		while(*dst_ip++ != ' ') i++;
		t_ptr = return_me_not_char(t_ptr, ' ');
		destin_ip = (char*)calloc(1, i+1);
		memcpy(destin_ip, src_ip, i);
		ip_frm->src_ip = enum_find_frame_type(source_ip, &IP_Address);
		if (ip_frm->src_ip == 0xFFFF){
			ip_frm->src_ip = enum_add(source_ip, &IP_Address);
		}
		 ip_frm->dst_ip = enum_find_frame_type(destin_ip, &IP_Address);
		if (ip_frm->dst_ip == 0xFFFF){
			ip_frm->dst_ip = enum_add(destin_ip, &IP_Address);
		}
		free(source_ip);
		free(destin_ip);
		t_ptr += i+1;
		t_ptr = return_me_not_char(t_ptr, ' ');
		// Proto extract
//		if (*t_ptr == ' ') t_ptr = return_me_not_char(t_ptr, ' ');
		i = 0;
		src_ip = t_ptr;
		dst_ip = t_ptr;
		while(*dst_ip++ != ' ') i++;
		source_ip = (char*)calloc(1, i+1);
		memcpy(source_ip, src_ip, i); // copied the proto
		ip_frm->protocol = enum_find_frame_type(source_ip, &Proto_Address);
		if (ip_frm->protocol == 0xFFFF){	// shift towards short, 0xFF is 255 protos, easily achieved.
			ip_frm->protocol = enum_add(source_ip, &Proto_Address);
		}
		free(source_ip);
		t_ptr += i+1;
		t_ptr = return_me_not_char(t_ptr, ' ');
		// extract the length field and terminate
		i = 0;
		src_ip = t_ptr;
		dst_ip = t_ptr;
		while(*dst_ip++ != ' ') i++;
		source_ip = (char*)calloc(1, i+1);
		memcpy(source_ip, src_ip, i);
		ip_frm->packet_size = atoi(source_ip);
		free(source_ip);
//		while(t_ptr++ != '\0');
	}
}
void process_wifi_frame(char *line, WiFi_Frame *wifi_frm){
        // use int to store src and dst
        char *t_ptr = line;

        while(*t_ptr++ != ' '); // skip the number
        while(*t_ptr++ != ' '); // skip the date
        while(*t_ptr++ != ' '); // skip the time
        if (*t_ptr == ' ' ){
		while(*t_ptr++ == ' '); // skip spaces
		t_ptr--;
	}
	// compensate the ++
        // do the stuff, extract the source
	// extract source mac
	if ((*t_ptr == (char)0x86) || (*t_ptr == (char)0x3e)){ // no source found, arrow 
		printf("\nWe have this unhandled case!");
	} else {
		//t_ptr++; // 
		char *start_mac = t_ptr;
		char *end_mac = t_ptr;
		unsigned char i = 0;
		while(*end_mac++ != ' ')i++; // get to the end of the mac address
			// copy the mac address and write down descriptor
		char *source_mac = (char*)calloc(1, i+1); // FREE
		char *destin_mac = 0x00;
		memcpy(source_mac, start_mac, i);
		t_ptr += i+1; // +1 skip space
		while(*t_ptr++ != ' ');
		i = 0;
		start_mac = t_ptr;
		end_mac = t_ptr;
		while(*end_mac++ != ' ') i++;
		destin_mac = (char*)calloc(1, i+1);
		memcpy(destin_mac, start_mac, i);
		// extracted source and dest
		wifi_frm->src_id = enum_find_frame_type(source_mac, &WiFi_Address);
		wifi_frm->dst_id = enum_find_frame_type(destin_mac, &WiFi_Address);
		if (wifi_frm->src_id == 0xFFFF){ // not found
			wifi_frm->src_id = enum_add(source_mac, &WiFi_Address);
		}
		if (wifi_frm->dst_id == 0xFFFF){ // not found
			wifi_frm->dst_id = enum_add(destin_mac, &WiFi_Address);
		}
		free(source_mac);
		free(destin_mac);
			// we have timestamp, src_id, dst_id, skip to len, then
		//special EAPOL packet case
		//
		while(*t_ptr++ != ' '); // get to padding
		while(*t_ptr++ == ' '); // skipp padding
		while(*t_ptr++ != ' '){
			if (eapol_flag == 1){
				t_ptr -= 2;
				if(manage_comparison(t_ptr, ' ', "EAPOL ")){
					start_mac = t_ptr;
			                end_mac = t_ptr;
			                i = 0;
			                while(*end_mac++ != ' ') i++; // get to the end
			                source_mac = (char*)calloc(1, i+1); // aloc
			                memcpy(source_mac, start_mac, i);
			                wifi_frm->frame_type = enum_find_frame_type(source_mac, &Enum_Start);
			                if (wifi_frm->frame_type == 0xFFFF){
			                       wifi_frm->frame_type = enum_add(source_mac, &Enum_Start);
			                }
			                free(source_mac); // frr
					t_ptr += i+1;
					start_mac = t_ptr;
			                end_mac = t_ptr;
			                i = 0;
			                while(*end_mac++ != ' ') i++; // example ...f3:4e:66:e6 EAPOL 151 Key (Message 1 of 4)00
			                source_mac = (char*)calloc(1, i+1); // aloc
			                memcpy(source_mac, start_mac, i);
			                wifi_frm->frame_length = atoi(source_mac);
			                free(source_mac); // frr
					eapol_flag = 0;
					return;
				}
			}
		} // skip 802.11
		// reusing variables
		start_mac = t_ptr;
		end_mac = t_ptr;
		i = 0;
		while(*end_mac++ != ' ')i++;
		source_mac = (char*)calloc(1, i+1); // allocate mem for length
		memcpy(source_mac, start_mac, i);
		wifi_frm->frame_length = atoi(source_mac);
		free(source_mac); // free allocated memory
		t_ptr += i+1;
		// got the length
			// frame types
		start_mac = t_ptr;
		end_mac = t_ptr;
		i = 0;
		while(*end_mac++ != ',') i++;
		source_mac = (char*)calloc(1, i+1); // aloc
		memcpy(source_mac, start_mac, i);
		wifi_frm->frame_type = enum_find_frame_type(source_mac, &Enum_Start);
		if (wifi_frm->frame_type == 0xFFFF){
			wifi_frm->frame_type = enum_add(source_mac, &Enum_Start);
		}
		free(source_mac); // frr
		// stored frame type
			// extract SN
		while(*t_ptr++ != '=');
		start_mac = t_ptr;
		end_mac = t_ptr;
		i=0;
		while(*end_mac++ != ','){ i++; if (*end_mac == ' '){ i--; break; }}
		source_mac = (char*)calloc(1, i+1);
		memcpy(source_mac, start_mac, i);
		wifi_frm->frame_sn = atoi(source_mac);
		free(source_mac);
		t_ptr += i+1;
			// extract FN
		while(*t_ptr++ != '=');
                start_mac = t_ptr;
                end_mac = t_ptr;
                i=0;
                while(*end_mac++ != ' '){ i++; if (*end_mac == ' '){ i--; break; }}
                source_mac = (char*)calloc(1, i+1);
                memcpy(source_mac, start_mac, i);
                wifi_frm->frame_fn = atoi(source_mac);
                free(source_mac);
		t_ptr += i+1;
			// process Flags if needed, for version 2 etc...
			// pointer currently is pointing at flags
		while(*t_ptr++ != '\n'){ if (*t_ptr == '\0'){ return; } else if (*t_ptr == ','){ break; } }
		// hacky solution to test ssid and store
		while(*t_ptr++ != 'S'){
			if ( *t_ptr == '\n' ||
				*t_ptr == '\0' ||
				*t_ptr == '\r'){
				// This case means that end of file or line, self-destruct ) 
				return;
			}
		}
		if (strncmp(t_ptr, "SID", 3) != 0) return; // it is not SSID it means return, failure
		// If I'm here I got the SSID
		t_ptr += 4; // Skip to number taking into account S[SID=]Mega
		start_mac = t_ptr;
                end_mac = t_ptr;
                i=0;
                while(*end_mac++ != '\0') i++;// if (*end_mac == '\0'){ i--; break; }}
                source_mac = (char*)calloc(1, i+1);
                memcpy(source_mac, start_mac, i);
                wifi_frm->bssid = source_mac;
		wifi_frm->bssid_len = i;
	}
}

WiFi_Frame **process_wifi_lines(char *ptr, unsigned long lines, unsigned int *filtered){
	char *line = ptr;
        char **t_upd = (char**)calloc(1, sizeof(t_upd));
        WiFi_Frame **wifi_arr = (WiFi_Frame**)calloc(lines, sizeof(wifi_arr));
        int wifi_arr_i = 0;
        unsigned int l_count = 0;
        while(l_count < lines){
                //printf("\n%08x\n", line);
                line = extract_line(line, t_upd);
                //printf("\nExtracted Line: %s", line);
                if (!validate_line_wifi(line)){
                        if (line == 0x00){
                //              printf("\nline==0x00\n");
                                free(line);
                                l_count++;
                                line = t_upd[0];
                                break;
                        }
                //      printf("line=t_upd\n");
			if(!validate_line_wifi_eapol(line)){
				free(line);
				l_count++;
				line = t_upd[0];
				continue;
			}
//                        free(line); l_count++;
//                        line = t_upd[0];
//                        continue;
                }
        //      printf("\nValidation went fine");
                wifi_arr[wifi_arr_i] = (WiFi_Frame*)calloc(1, sizeof(WiFi_Frame));
                wifi_arr[wifi_arr_i]->timestamp = convert_date_to_epoch(line);
                process_wifi_frame(line, wifi_arr[wifi_arr_i]);
                free(line);
                line = t_upd[0];
                wifi_arr_i++;
                l_count++;
        }
        *filtered = wifi_arr_i;
        free(t_upd);
        return wifi_arr;
}
unsigned char validate_ip_short(char *line){
	unsigned char validate = 0;
	while(*line++ != '\0'){
		if (*line == '.') validate++;
		if (validate == 7) return 1;
		// if arp break
		if (*line == 'A'){
			if(*(line+1) == 'R'){
				if (*(line+2) == 'P'){
					return 0;
				}
			}
		}
		// if MDNS the case of IPv6
		if (*line == 'M'){
                        if(*(line+1) == 'D'){
                                if (*(line+2) == 'N'){
                                        if (*(line+3) == 'S'){
						return 0;
					}
                                }
                        }
                }
	}
	return 0;

}
unsigned int line_counte = 0;
unsigned int invalid = 0;
unsigned int _invalid= 0;
char *global_deb_string;
//        1
// 10000000
IP_Frame **process_ip_frame_lines(char *ptr, unsigned long lines, unsigned int *filtered){
	char *line = ptr;
	global_deb_string = ptr;
	char **t_upd = (char**)calloc(1, sizeof(t_upd));
	IP_Frame **ip_arr = (IP_Frame**)calloc(lines, sizeof(ip_arr));
	int ip_arr_i = 0;
	unsigned int l_count = 0;
	while(l_count < lines){
		line = extract_line(line, t_upd);
		if (!validate_ip_short(line)){
			if (line == 0x00){
				free(line);
				l_count++;
				line = t_upd[0]; invalid++;
				break;
			}
			_invalid++;
			free(line); l_count++;
			line = t_upd[0];
			continue;
		}
		line_counte++;
		ip_arr[ip_arr_i] = (IP_Frame*)calloc(1, sizeof(IP_Frame));
		ip_arr[ip_arr_i]->timestamp = convert_date_to_epoch(line);
		process_ip_frame(line, ip_arr[ip_arr_i]);
		free(line);
		line = t_upd[0];
		ip_arr_i++;
		l_count++;
	}
	*filtered = --ip_arr_i;
        free(t_upd);
        return ip_arr;
}
ZigBee_Frame **process_zigbee_lines(char *ptr, unsigned long lines, unsigned int *filtered){
	char *line = ptr;
	char **t_upd = (char**)calloc(1, sizeof(t_upd));
	ZigBee_Frame **zb_arr = (ZigBee_Frame**)calloc(lines, sizeof(zb_arr));
	int zb_arr_i = 0;
	unsigned int l_count = 0;
	while(l_count < lines){
		//printf("\n%08x\n", line);
		line = extract_line(line, t_upd);
		//printf("\nExtracted Line: %s", line);
		if (!validate_line_zigbee(line)){
			if (line == 0x00){
		//		printf("\nline==0x00\n");
				free(line);
				l_count++;
				line = t_upd[0];
				break;
			}
		//	printf("line=t_upd\n");
			free(line); l_count++;
			line = t_upd[0];
 			continue;
		}
	//	printf("\nValidation went fine");
		zb_arr[zb_arr_i] = (ZigBee_Frame*)calloc(1, sizeof(ZigBee_Frame));
		zb_arr[zb_arr_i]->timestamp = convert_date_to_epoch(line);
		exctract_source_id(line, zb_arr[zb_arr_i]);
		free(line);
		line = t_upd[0];
		zb_arr_i++;
		l_count++;
	}
	*filtered = --zb_arr_i;
	free(t_upd);
	return zb_arr;
}

Audio_Frame **process_audio_frame_lines(char *ptr, unsigned long lines, unsigned int *filtered){
	char *line = ptr;
	char **t_upd = (char**)calloc(1, sizeof(t_upd));
	Audio_Frame **audio_arr = (Audio_Frame**)calloc(lines, sizeof(audio_arr));
	int audio_arr_i = 0;
	unsigned int l_count = 0;
	while(l_count < lines){
		line = extract_line(line, t_upd);
		// the format is csv 0.16003552981111932,2017-11-24,15:14:03
		// value,date,time
		audio_arr[audio_arr_i] = (Audio_Frame*)calloc(1, sizeof(Audio_Frame));
		char *start = line;
		char *t_ptr = 0;
		char *tt_ptr= 0;
		char *test=0;
		int i = 0;
		int z = 0;
		while(*(start++) != ',')i++;
		start = (char*)calloc((i+1), sizeof(char));
		memcpy(start, line, i); // -1 removes the comma
		audio_arr[audio_arr_i]->db = atof(start);
		free(start);
		start = line+i+1; i = 0; // +1 because currently pointing at ,
		while(*(start++) != ',')i++;
		t_ptr = (char*)calloc(i+1, sizeof(char));
		memcpy(t_ptr, (start-(1+i)), i);
		//start is currently at the last bit
		tt_ptr = start;
		i=0;
		while(*(tt_ptr++) != ','){i++; if(*tt_ptr == '\r' || *tt_ptr == '\n')break; } // got the last bit
		test=(char*)calloc(i+1, sizeof(char));
		memcpy(test, start, i);
		// now we have to merge them together
		// t_ptr = date, z ?= date length
		// test = time, i = time length
		tt_ptr = t_ptr;
		while(*tt_ptr++ != '\0') z++;
		start = (char*)calloc(z+i+1+12, sizeof(char)); // Why 3? Why not I would say )) 
		sprintf(start, "1 %s %s.000000  a",t_ptr, test);
		audio_arr[audio_arr_i]->timestamp = convert_date_to_epoch(start);
		memcpy(start, t_ptr, z);
		memcpy(start+z, " ", 1); // add space
		memcpy(start+z+1, test, i);
		free(start);
		free(test);
		free(t_ptr);
		audio_arr_i++;
		l_count++;
	}
	*filtered = --audio_arr_i;
	free(t_upd);
	return audio_arr;
}
void process_audio_input_live(unsigned char live_mode, char *line_buffer){
	unsigned int line_count = 0;
	unsigned int filtered = 0;
	Audio_Frame **audio_arr_ptr = process_audio_frame_lines(line_buffer, 1, &filtered);
	if (filtered != 0xFFFFFFFF) write_out_frames_new((void*)audio_arr_ptr, 1, '\0', line_count);
//	if ((filtered != 0xFFFFFFFF) && (live_descriptor_write)){write_descriptor(&IP_Address, "ip_addr_descriptor.csv"); write_descriptor(&Proto_Address, "ip_proto_descriptor.csv"); live_descriptor_write = 0;}
}
void process_ip_short_input_live(unsigned char live_mode, char *line_buffer){
	unsigned int line_count = 0;
	unsigned int filtered = 0;
	IP_Frame **ip_arr_ptr = process_ip_frame_lines(line_buffer, 1, &filtered);
	if (filtered != 0xFFFFFFFF) write_out_frames_new((void*)ip_arr_ptr, 1, '\0', line_count);
	if ((filtered != 0xFFFFFFFF) && (live_descriptor_write)){
	write_descriptor(&IP_Address, "addresses.csv"); write_descriptor(&Proto_Address, "protocols.csv"); live_descriptor_write = 0;}
}
void process_zigbee_file_input_live(unsigned char live_mode, char *line_buffer){
	unsigned int line_count = 0;
	unsigned int filtered = 0;
	ZigBee_Frame **zb_arr_ptr = process_zigbee_lines(line_buffer, 1, &filtered);
	if (filtered != 0xFFFFFFFF) write_out_frames_new((void*)zb_arr_ptr, 1,'\0', line_count);
	if ((filtered != 0xFFFFFFFF) && (live_descriptor_write)){write_descriptor(&Enum_Start, "protocols.csv"); live_descriptor_write = 0;}
}
void process_wifi_file_input_live(unsigned char live_mode, char *line_buffer){
	unsigned int line_count = 0;
	unsigned int filtered = 0;
	WiFi_Frame **wifi_arr_ptr= process_wifi_lines(line_buffer, 1, &filtered);
	if (filtered != 0xFFFFFFFF) write_out_frames_new((void*)wifi_arr_ptr, 1,'\0', line_count);
        if ((filtered != 0xFFFFFFFF) && (live_descriptor_write)){write_descriptor(&Enum_Start, "protocols.csv"); write_descriptor(&WiFi_Address, "addresses.csv"); live_descriptor_write = 0;}
}
void process_zigbee_file_input(FILE *file, unsigned long number_of_lines){
	char *ptr = 0;
	unsigned int line_count = 0;
	unsigned int filtered = 0;
	FILE *out_file = open_file(out_filename_ptr, "w+");
	fwrite(&Version, sizeof(int), 1, out_file);
	fwrite(&line_count, sizeof(int), 1, out_file);
	//fwrite(&zb_frame_len, sizeof(int), 1, out_file);
	fclose(out_file);

	printf("Started processing ZigBee Input File!");
	//
	long file_content_line_slot = number_of_lines/100;
	if (file_content_line_slot == 0) file_content_line_slot = number_of_lines ; // less than a hundred, assign number of lines
	unsigned long next_read = 0;
	unsigned long slot = file_content_line_slot;
	unsigned long overall_filtered = 0;
	while(next_read < number_of_lines){
		ptr = extract_number_of_lines(file, next_read, next_read+slot);
		ZigBee_Frame **zb_arr_ptr = process_zigbee_lines(ptr, slot, &filtered);
		write_out_frames_new((void*)zb_arr_ptr, filtered, '.', line_count);
		next_read += file_content_line_slot;
		free(ptr);
		overall_filtered += filtered;
		if ((next_read+slot) > number_of_lines){ slot = number_of_lines-next_read; }
		if ((filtered != 0xFFFFFFFF) && (live_descriptor_write)){write_descriptor(&Enum_Start, "protocols.csv"); live_descriptor_write = 0;}
	}
}
/*
char *extract_n_lines(FILE *file, long n_lines, long *offset){
	process_zigbee_file_input(file, n_lines);
	char *ptr = 0;
	unsigned int line_count = 0;
	long char_pos = 0;
	int cur_line_len = 0;
	unsigned int zb_frame_len = sizeof(ZigBee_Frame);
	FILE *out_file = open_file(out_filename_ptr, "w+");
	fwrite(&Version, sizeof(int), 1, out_file);
	fwrite(&line_count, sizeof(int), 1, out_file);
	//fwrite(&zb_frame_len, sizeof(int), 1, out_file);
	fclose(out_file);
	ZigBee_Frame *zb_arr[1000];
	int zb_arr_i = 0;
	printf("Start\n");
	while(line_count != n_lines){
		zb_arr[zb_arr_i] = (ZigBee_Frame*)calloc(1, sizeof(ZigBee_Frame));
		ptr = extract_next_line(file, &cur_line_len);
		if (!validate_line_zigbee(ptr)){ if (ptr == 0x00){free(ptr); break;}free(ptr); free(zb_arr[zb_arr_i]); continue;}
		zb_arr[zb_arr_i]->timestamp = convert_date_to_epoch(ptr);
		exctract_source_id(ptr, zb_arr[zb_arr_i]);
		free(ptr);
		zb_arr_i++;
		line_count++;
		if (zb_arr_i == 1000){
			write_out_frames((void*)zb_arr, zb_arr_i, '.', line_count);
			zb_arr_i = 0;
		}
		//free(ret_ptr);
	}
	write_out_frames((void*)zb_arr, zb_arr_i, '.', line_count);
	write_descriptor(&Enum_Start, "descriptor.csv");
	printf("\nOverall lines: %d, Processed: %d\n", n_lines, line_count);
	return ptr;
}
*/
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

void process_wifi_file_input(FILE *file, unsigned long number_of_lines){
	char *ptr = 0;
        unsigned int line_count = 0;
        unsigned int filtered = 0;
        FILE *out_file = open_file(out_filename_ptr, "w+");
        fwrite(&Version, sizeof(int), 1, out_file);
        fwrite(&line_count, sizeof(int), 1, out_file);
        //fwrite(&zb_frame_len, sizeof(int), 1, out_file);
        fclose(out_file);

        printf("Started processing WiFi Input File!\n");
        //
        unsigned long file_content_line_slot = number_of_lines/100;
        if (file_content_line_slot == 0) file_content_line_slot = number_of_lines ; // less than a hundred, assign number of lines
        unsigned long next_read = 0;
        unsigned long slot = file_content_line_slot;
        unsigned long overall_filtered = 0;
        while(next_read < number_of_lines){
                ptr = extract_number_of_lines(file, next_read, next_read+slot);
                //ZigBee_Frame **zb_arr_ptr = process_zigbee_lines(ptr, slot, &filtered);
		WiFi_Frame **wifi_arr_ptr = process_wifi_lines(ptr, slot, &filtered);
		printf("Processed this number of lines: %u (%lu/%lu)\n", filtered, next_read, next_read+slot);
                write_out_frames_new((void*)wifi_arr_ptr, filtered, '.', line_count);
                next_read += file_content_line_slot;
                free(ptr);
                overall_filtered += filtered;
                if ((next_read+slot) > number_of_lines){ slot = number_of_lines-next_read; }
                if ((filtered != 0xFFFFFFFF) && (live_descriptor_write)){write_descriptor(&Enum_Start, "protocols.csv"); write_descriptor(&WiFi_Address, "addresses.csv"); live_descriptor_write = 0;}
        }
}

int main(int argc, char *argv[])
{
	//printf("Hello World, %s\n", argv[0]);
	path = extract_path(argv[0]);
	enum_init(&Enum_Start);
	enum_init(&WiFi_Address);
	enum_init(&IP_Address);
	enum_init(&Proto_Address);
	//load_maps();
	unsigned short parameter_flags = argument_flagger(argc, argv);
	load_maps();
	if (((parameter_flags & HELP_FLAG) != 0)){ showHelpMessage(); return 0; }
	if (((parameter_flags & ZIGB_FLAG) != 0) &&	//
		((parameter_flags & IN_F_FLAG) != 0) && 
		((parameter_flags & OU_F_FLAG) != 0)){
			FILE *in_file = open_file(in_filename_ptr, "r");
			if (in_file != NULL){
				// Initial line offsets 100k
				line_offsets = (uint64_t*)calloc(LINE_INIT+1, sizeof(uint64_t));
				*line_offsets = 0; // first starts at offset zero
				line_offset_index++;
				uint64_t lines = get_number_of_lines(in_file);
				fseek(in_file, 0, SEEK_SET);
				//char *test = extract_n_lines(in_file, line_p_session, 0);
				process_zigbee_file_input(in_file, lines);
				//process_zigbee_file_input_live(LIVE_FLAG, line_buffer);
				printf("Was able to open file!\nAllocs: %lu Freed: %lu\n", mlk_alloc, mlk_free);
				fclose(in_file);
			}

	} else if (((parameter_flags & LIVE_FLAG) != 0) &&	//
				((parameter_flags & OU_F_FLAG) != 0) &&
				(((parameter_flags & ZIGB_FLAG) == 0) || ((parameter_flags & WIFI_FLAG) == 0))){

				char *line_buffer = (char*)calloc(1024, sizeof(char));
				unsigned int line_count = 0;
				FILE *out_file = open_file(out_filename_ptr, "w+");
				fwrite(&Version, sizeof(int), 1, out_file);
				fwrite(&line_count, sizeof(int), 1, out_file);
				fclose(out_file);
				while(fgets(line_buffer, 1024, stdin) != NULL){
					if (((parameter_flags & ZIGB_FLAG) == ZIGB_FLAG)){
						//printf("\nEn - %s\n", line_buffer);
						process_zigbee_file_input_live(LIVE_FLAG, line_buffer);

					} else if (((parameter_flags & WIFI_FLAG) == WIFI_FLAG)){
						process_wifi_file_input_live(LIVE_FLAG, line_buffer);
					} else if (((parameter_flags & IP_SHOR_F) == IP_SHOR_F)){
						process_ip_short_input_live(LIVE_FLAG, line_buffer);
					} else if (((parameter_flags & AUDIO_FLA) == AUDIO_FLA)){
						process_audio_input_live(LIVE_FLAG, line_buffer);
					}
					line_count++;
				}

	} else if (((parameter_flags & WIFI_FLAG) != 0) &&     //
                	((parameter_flags & IN_F_FLAG) != 0) &&
                	((parameter_flags & OU_F_FLAG) != 0)){

			FILE *in_file = open_file(in_filename_ptr, "r");
                        if (in_file != NULL){
                                // Initial line offsets 100k
                                line_offsets = (uint64_t*)calloc(LINE_INIT+1, sizeof(uint64_t));
                                *line_offsets = 0; // first starts at offset zero
                                line_offset_index++;
                                uint64_t lines = get_number_of_lines(in_file);
                                fseek(in_file, 0, SEEK_SET);
                                //char *test = extract_n_lines(in_file, line_p_session, 0);
                                process_wifi_file_input(in_file, lines);
                                printf("Was able to open file!\nAllocs: %lu Freed: %lu\n", mlk_alloc, mlk_free);
                                fclose(in_file);
                        }
	} else {
		showHelpMessage();
	}
//	cin.get();
	return 0;
}

