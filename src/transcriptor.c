#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "structures.h"

#define INTERVAL 1000000 // second

typedef struct FRAME{
	unsigned int frame_len;
	char *frame_ptr;
	struct FRAME *next;
}FRAME;
typedef struct SLOT{
	uint64_t slot_start_time;
	unsigned int n;
	FRAME *frame_array;
	uint64_t slot_stop_time;
}SLOT;

uint64_t global_start_time = 0;
int     slot_parsed_counter = 0;

unsigned char filter_address(void *object, unsigned char type, unsigned short exclude_addr){
	switch(type){
		case 1:
			//ZigBee_Frame *obj = (ZigBee_Frame*)object;
			if (((ZigBee_Frame*)object)->src_id == exclude_addr) return 0;
			if (((ZigBee_Frame*)object)->dst_id == exclude_addr) return 0;
			// parse ZigBee
			break;
		case 2:
			//IP_Frame *obj = (IP_Frame*)object;
			if (((IP_Frame*)object)->src_ip == exclude_addr) return 0;
			if (((IP_Frame*)object)->dst_ip == exclude_addr) return 0;
			// parse IP_Short
			break;
		case 3:
			//WiFi_Frame *obj = (WiFi_Frame*)object;
			if (((WiFi_Frame*)object)->src_id == exclude_addr) return 0;
			if (((WiFi_Frame*)object)->dst_id == exclude_addr) return 0;
			// parse wifi
			break;
		case 4:
			// parse audio
			break;
	}
	return 1;
}
SLOT* slot_init(){
	SLOT *ptr = (SLOT*)calloc(1, sizeof(SLOT));
	return ptr;
}
void free_slot(SLOT *slot){
	char *ptr;
	FRAME *frame_ptr;
	FRAME *min_one;
	frame_ptr = slot->frame_array;
	min_one = slot->frame_array;
	while(slot->n--){
		while(frame_ptr->next != 0){min_one = frame_ptr; frame_ptr = frame_ptr->next;}
		free(frame_ptr->frame_ptr);
		free(frame_ptr);
		min_one->next = 0;
		frame_ptr = slot->frame_array;
		printf(".");
		fflush(stdout);
	}
	slot->n = 0;
	slot->frame_array = 0;
}
void frame_add(SLOT *slot, void *object, unsigned char object_size, unsigned char type){
	// type 0 = wifi, 1 = zigbee, 2 = ip, 3 = sound
	FRAME *_frame = (FRAME*)0x0000;
	switch(type){
		case 0:
			//wifi
			
			break;
		case 1:
			//zigbee
			if (slot->frame_array != 0){
				_frame = slot->frame_array;
				while(_frame->next != 0) _frame = _frame->next;
				_frame->next = (FRAME *)calloc(1, sizeof(FRAME));
				_frame->next->frame_len = object_size;// zigbee frame length
				_frame->next->frame_ptr = (char*)calloc(1, _frame->next->frame_len);
				memcpy(_frame->next->frame_ptr, object, _frame->next->frame_len);
				slot->n++;
			} else {
                                _frame = (FRAME *)calloc(1, sizeof(FRAME));
                                _frame->frame_len = object_size;// zigbee frame length
                                _frame->frame_ptr = (char*)calloc(1, _frame->frame_len);
                                memcpy(_frame->frame_ptr, object, _frame->frame_len);
				slot->frame_array = _frame;
                                slot->n++;
//				printf("\n%d\n", ((ZigBee_Frame*)object)->packet_size);
			}

			break;
		case 2:
			//ip
			break;
		case 3:
			//sound
			break;
		default:
			break;
	}
}
void update_stop_start_times(SLOT *slot){
	slot->slot_start_time = global_start_time + (INTERVAL*slot_parsed_counter++);
        slot->slot_stop_time = slot->slot_start_time+INTERVAL;
}
int main(int argc, char **argv){ int mode = 1;
	FILE *file = fopen(argv[1], "rb");
	if (argc > 2){
		mode = atoi(argv[2]);
	}
	int version = 0;
	int pkt_number = 0;
	unsigned char slot_index;
	fseek(file, 0, SEEK_SET);
	fread(&version, sizeof(int), 1, file);
	fread(&pkt_number,sizeof(int), 1,file);
//	printf("Version: %d Packets: %d\n", version, pkt_number);
	SLOT *slot[2];
	slot[0] = slot_init();
	slot[1] = slot_init();
	while(pkt_number--){
		int t_len = 0;
		if (mode == 0){	// Wifi
			WiFi_Frame t_wf_frm;
			fread(&t_len, sizeof(char), 1, file);
			fread(&t_wf_frm, t_len, 1, file);
			printf("%"PRIu64",%04x,%04x,%d,%d,%d\n",
				t_wf_frm.timestamp, t_wf_frm.src_id, t_wf_frm.dst_id,
				t_wf_frm.bssid_len, t_wf_frm.essid_len, t_wf_frm.frame_type);
		} else if (mode == 1){	// ZigBee
			ZigBee_Frame t_zb_frm;
        	        fread(&t_len, sizeof(char), 1, file);
        	        fread(&t_zb_frm, t_len, 1, file);
			if (slot[slot_index]->n == 0){
				if (global_start_time == 0) global_start_time = t_zb_frm.timestamp;
				update_stop_start_times(slot[slot_index]);
				//slot->frame_array = (FRAME*)calloc(1, sizeof(FRAME));
				//slot->frame_array->frame_len = t_len;
				//slot->frame_array->frame_ptr = (char*)calloc(1, t_len);
				//memcpy(slot->frame_array->frame_ptr, &t_zb_frm, t_len);
				//slot->n++;
				frame_add(slot[slot_index], &t_zb_frm, t_len, 1);
				continue;
			} else {
				if ((t_zb_frm.timestamp >= slot[slot_index]->slot_start_time)&&(t_zb_frm.timestamp <= slot[slot_index]->slot_stop_time)){
				//	FRAME *frame = (FRAME*)calloc(1, sizeof(FRAME));
			//		FRAME *t_frame = slot->frame_array;
			//		while(t_frame->next != 0) t_frame = t_frame->next;
					// I have the last frame
			//		t_frame->next = frame;
			//		frame->frame_len = t_len;
			//		frame->frame_ptr = (char*)calloc(1, t_len);
			//		memcpy(frame->frame_ptr, &t_zb_frm, t_len);
			//		slot->n++;
					
					frame_add(slot[slot_index], &t_zb_frm, t_len, 1);
				} else { // the timestamp is higher
					// do stat analysis
					printf("\ngot a slot, pkt: %d", slot[slot_index]->n);
					if (slot_index == 0){
						 slot_index++;
					} else if (slot_index == 1){
						slot_index++;
					}
						// output
					if (slot_index == 2){
						free_slot(slot[0]);
						free_slot(slot[1]);
						slot_index = 0;
					}

//					free_slot(slot);
	//				return 0;
					// free slot

					// re-initialize
				}
				continue;
			}
	// if (t_zb_frm.frame_type == 0) continue;
	// printf("%04x\n",t_zb_frm.src_id);
	                printf("%"PRIu64",%04x,%04x,%d,%d,%d\n",
        	                t_zb_frm.timestamp, t_zb_frm.src_id, t_zb_frm.dst_id,
                	        t_zb_frm.frame_type, t_zb_frm.packet_size, t_zb_frm.flags);
	// if (t_zb_frm.flags != 1) inc_relation(Start, t_zb_frm.src_id, t_zb_frm.dst_id);
		} else if (mode == 2){	// IP Short
			IP_Frame t_ip_frm;
        	        fread(&t_len, sizeof(char), 1, file);
	                fread(&t_ip_frm, sizeof(IP_Frame), 1, file);
	// if (t_zb_frm.frame_type == 0) continue;
	// printf("%04x\n",t_zb_frm.src_id);
	                printf("%"PRIu64",%04x,%04x,%d,%d\n",
	                        t_ip_frm.timestamp, t_ip_frm.src_ip, t_ip_frm.dst_ip,
	                        t_ip_frm.protocol, t_ip_frm.packet_size);//, t_zb_frm.flags);
	// if (t_zb_frm.flags != 1) inc_relation(Start, t_zb_frm.src_id, t_zb_frm.dst_id);
		} else if (mode == 3){	// Sound else {
			printf("Mode not supported\n");
		}
	}
// Relation *ptr = Start; // while(ptr->next != 0){ // printf("Relation: %04x -> %04x. Hits: %lu\n", ptr->src_id, ptr->dst_id, ptr->counter); // ptr = ptr->next; //	} //	printf("Debug break");
	return 0;
}
