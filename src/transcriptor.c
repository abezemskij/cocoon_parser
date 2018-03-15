#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "structures.h"

#define INTERVAL 1000000//1000000 // second

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
typedef struct SOURCE_ENUM{
	unsigned short *array;
	unsigned short n;
}SOURCE_ENUM;

uint64_t global_start_time = 0;
int     slot_parsed_counter = 0;
unsigned long mallocs = 0;
unsigned long frees = 0;
SOURCE_ENUM Global_Sources;
SOURCE_ENUM Global_Destinations;
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
			if (((IP_Frame*)object)->src_id == exclude_addr) return 0;
			if (((IP_Frame*)object)->dst_id == exclude_addr) return 0;
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
		//frees++;
		min_one->next = 0;
		frame_ptr = slot->frame_array;
		//printf(".");
		fflush(stdout);
	}
	//printf("\n");
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
			//mallocs++;
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
/*void process_zigbee(SLOT *slot){
	unsigned short ids[slot->n];
	unsigned char frames[slot->n];
	unsigned short parsed_ids[slot->n];
	unsigned char parsed_frames[slot->n];
	unsigned int i = 0;
	FRAME *_frame = slot->frame_array;
	while(i < slot->n){ // enumerate ids
		ZigBee_Frame *frm = (ZigBee_Frame*)_frame->frame_ptr;
		ids[i] = frm->src_id;
		printf("id: %d src: %04x\n", i, ids[i]);
		_frame = _frame->next;
		i++;
	}
	i = 0;
}*/
unsigned short get_unique_number_wi(SLOT *slot){   // enumerate id's
        unsigned int i = 0;
        unsigned short unique_src = 0;
        unsigned short unique_dst = 0;
        FRAME *_frame = slot->frame_array;
        unsigned short *src_array = 0;
        unsigned short *dst_array = 0;
        while(i < slot->n){
                WiFi_Frame *frm = (WiFi_Frame*)_frame->frame_ptr;
                // do for sources
                if (unique_src == 0){
                        // save the first source
                        if (frm != 0){
                                src_array = (unsigned short*)calloc(1, sizeof(short));
                                *src_array = frm->src_id;
                                unique_src++;
                        } else {
                                printf("ERROR: Accessed get_unique_number [frm == 0] (src)\n");
                        }
                } else {
                        unsigned short _t = 0;
                        unsigned short *_ptr = src_array;
                        unsigned char found = 0;
                        while(_t < unique_src){
                                if (_ptr[_t] == frm->src_id){
                                        found = 1;
                                        break;
                                }
                                _t++;
                        }
                        if (found != 1){ // if src was not found, add
                                unique_src++;   // this will be added
                                unsigned short *free_me = src_array;
                                unsigned short *upd_src_array = (unsigned short*)calloc(unique_src, sizeof(short)); // allocate known unique +1
                                memcpy(upd_src_array, free_me, sizeof(short)*(unique_src-1));
                                free(free_me);
                                free_me = upd_src_array+unique_src-1;
                                *free_me = frm->src_id;
                                src_array = upd_src_array;
                        }
                }
                // do for destinations
                if (unique_dst == 0){
                        if (frm != 0){
                                dst_array = (unsigned short*)calloc(1, sizeof(short));
                                *dst_array= frm->dst_id;
                                unique_dst++;
                        } else {
                                printf("ERROR: Accessed get_unique_number [frm == 0] (dst)\n");
                        }
                } else {
                        unsigned short _t = 0;
                        unsigned short *_ptr = dst_array;
                        unsigned char found = 0;
                        while(_t < unique_dst){
                                if (_ptr[_t] == frm->dst_id){
                                        found = 1;
                                        break;
                                }
                                _t++;
                        }
                        if (found != 1){
                                unique_dst++;
                                unsigned short *free_me = dst_array;
                                unsigned short *upd_dst_array = (unsigned short*)calloc(unique_dst, sizeof(short));
                                memcpy(upd_dst_array, free_me, sizeof(short)*(unique_dst-1));
                                free(free_me);
                                free_me = upd_dst_array+unique_dst-1;
                                *free_me = frm->dst_id;
                                dst_array = upd_dst_array;
                        }
                }
                i++;
                _frame = _frame->next;
        }
        //if (src_array != 0) free(src_array);
        Global_Sources.array = src_array;
        Global_Destinations.array = dst_array;
        Global_Destinations.n = unique_dst;
        Global_Sources.n = unique_src;
        return unique_src;
}

unsigned short get_unique_number(SLOT *slot){	// enumerate id's
	unsigned int i = 0;
	unsigned short unique_src = 0;
	unsigned short unique_dst = 0;
	FRAME *_frame = slot->frame_array;
	unsigned short *src_array = 0;
	unsigned short *dst_array = 0;
	while(i < slot->n){
		ZigBee_Frame *frm = (ZigBee_Frame*)_frame->frame_ptr;
		// do for sources
		if (unique_src == 0){
			// save the first source
			if (frm != 0){
				src_array = (unsigned short*)calloc(1, sizeof(short));
				*src_array = frm->src_id;
				unique_src++;
			} else {
				printf("ERROR: Accessed get_unique_number [frm == 0] (src)\n");
			}
		} else {
			unsigned short _t = 0;
			unsigned short *_ptr = src_array;
			unsigned char found = 0;
			while(_t < unique_src){
				if (_ptr[_t] == frm->src_id){
					found = 1;
					break;
				}
				_t++;
			}
			if (found != 1){ // if src was not found, add
				unique_src++;	// this will be added
				unsigned short *free_me = src_array;
				unsigned short *upd_src_array = (unsigned short*)calloc(unique_src, sizeof(short)); // allocate known unique +1
				memcpy(upd_src_array, free_me, sizeof(short)*(unique_src-1));
				free(free_me);
				free_me = upd_src_array+unique_src-1;
				*free_me = frm->src_id;
				src_array = upd_src_array;
			}
		}
		// do for destinations
		if (unique_dst == 0){
			if (frm != 0){
				dst_array = (unsigned short*)calloc(1, sizeof(short));
				*dst_array= frm->dst_id;
				unique_dst++;
			} else {
				printf("ERROR: Accessed get_unique_number [frm == 0] (dst)\n");
			}
		} else {
			unsigned short _t = 0;
			unsigned short *_ptr = dst_array;
			unsigned char found = 0;
			while(_t < unique_dst){
				if (_ptr[_t] == frm->dst_id){
					found = 1;
					break;
				}
				_t++;
			}
			if (found != 1){
				unique_dst++;
				unsigned short *free_me = dst_array;
				unsigned short *upd_dst_array = (unsigned short*)calloc(unique_dst, sizeof(short));
				memcpy(upd_dst_array, free_me, sizeof(short)*(unique_dst-1));
				free(free_me);
				free_me = upd_dst_array+unique_dst-1;
				*free_me = frm->dst_id;
				dst_array = upd_dst_array;
			}
		}
		i++;
		_frame = _frame->next;
	}
	//if (src_array != 0) free(src_array);
	Global_Sources.array = src_array;
	Global_Destinations.array = dst_array;
	Global_Destinations.n = unique_dst;
	Global_Sources.n = unique_src;
	return unique_src;
}
void _math_minmax(unsigned int *array, unsigned int n, unsigned int *min, unsigned int *max){
	unsigned int _min = INT_MAX;
	unsigned int _max = 0;
	unsigned int i = 0;
	while (i < n){
		if (_min > array[i]) _min = array[i];
		if (_max < array[i]) _max = array[i];
		i++;
	}
	*min = _min;
	*max = _max;

}
void _math_minmax_dbl(double *array, unsigned int n, double *min, double *max){
	double _min = 9999999999.0;
	double _max = 0;
	unsigned int i = 0;
	while (i < n){
                if (_min > array[i]) _min = array[i];
                if (_max < array[i]) _max = array[i];
                i++;
        }
        *min = _min;
        *max = _max;
}
double _math_average(unsigned int *array, unsigned int n){
	double result = 0.0;
	unsigned int i = 0;
	while(i < n){
		result += array[i];
		i++;
	}
	result = result/n;
	return result;
}
double _math_average_dbl(double *array, unsigned int n){
        double result = 0.0;
        unsigned int i = 0;
        while(i < n){
                result += array[i];
                i++;
        }
        result = result/n;
        return result;
}
double _math_variance(unsigned int *array, double average, unsigned int n){
	double result = 0.0;
	unsigned i = 0;
	while(i < n){
		result += pow(((double)array[i]-average), 2); 
		i++;
	}
	result = result/(n-1);
	return result;
}
double _math_variance_dbl(double *array, double average, unsigned int n){
        double result = 0.0;
        unsigned i = 0;
        while(i < n){
                result += pow(((double)array[i]-average), 2); 
                i++;
        }
        result = result/(n-1);
        return result;
}

double _math_stdev(double variance){
	double result;
	result = sqrt(variance);
	return result;
}

double _math_avg_dev(unsigned int *array, unsigned int n){
	double result;
	unsigned int i = 1;
	while(i < n){
		result = fabs((double)array[i]-(double)array[i-1]);
		i++;
	}
	result = result/(n-1);
	return result;
}
double _math_avg_dev_dbl(double *array, unsigned int n){
        double result;
        unsigned int i = 1;
        while(i < n){
                result = fabs((double)array[i]-(double)array[i-1]);
                i++;
        }
        result = result/(n-1);
        return result;
}
void process_audio(SLOT *slot){
        unsigned int i = 0;
        unsigned int k = 0;
        unsigned int j = 0;
        double avg = 0.0;
        double stdev = 0.0;
        double avg_dev = 0.0;
        FRAME *_frame = slot->frame_array;
	double *val_array = (double*)calloc(slot->n, sizeof(double));
	while(i < slot->n){
		Audio_Frame *frm = (Audio_Frame*)_frame->frame_ptr;
		val_array[i] = frm->db;
		avg += frm->db;
		_frame = _frame->next;
		i++;
	}
	if (i != 0){
		double test = avg/i;
		avg = _math_average_dbl(val_array, i);
		stdev = _math_stdev(_math_variance_dbl(val_array, avg, i));
		avg_dev = _math_avg_dev_dbl(val_array, i);
		if (isnan(avg_dev)) avg_dev=0.0;
		if (isnan(stdev)) stdev = 0.0;
		double min = 0.0;
		double max = 0.0;
		_math_minmax_dbl(val_array, i, &min, &max);
		printf("%"PRIu64",%04x,%04x,555,%d,%f,%f,%f,%f,%f\n", slot->slot_stop_time,
			1, 1, i, avg, stdev, avg_dev, min, max);
		avg = 0.0;
		stdev = 0.0;
		avg_dev = 0.0;
		free(val_array);
	}
}
void process_wifi_ap(SLOT *slot){
        unsigned int i = 0;
        unsigned int k = 0;
        unsigned int j = 0;
        unsigned int freq = 0;
        double avg = 0.0;
        double stdev = 0.0;
        double avg_dev = 0.0;
        FRAME *_frame = slot->frame_array;
        while (k < Global_Sources.n){   // for every source
                while(j < Global_Destinations.n){ // for every destination
                        _frame = slot->frame_array;
                        unsigned int *val_array = (unsigned int*)calloc(slot->n, sizeof(int)); // worst case scenario
                        while(i < slot->n){ // for each packet
                                ZigBee_Frame *frm = (ZigBee_Frame*)_frame->frame_ptr;
                                if ((*(Global_Sources.array+k) == frm->src_id) && (*(Global_Destinations.array+j) == frm->dst_id)){
                                        val_array[freq] = frm->packet_size;
                                        freq++;
                                        avg += frm->packet_size;
                                }
                                _frame = _frame->next;
                                i++;
                        }
                        double test = avg/freq;
                        avg = _math_average(val_array, freq);
                        stdev = _math_stdev(_math_variance(val_array, avg, freq));
                        avg_dev = _math_avg_dev(val_array, freq);
                        if (isnan(avg_dev)) avg_dev=0.0;
                        if (isnan(stdev)) stdev = 0.0;
                        unsigned int min = 0;
                        unsigned int max = 0;
                        _math_minmax(val_array, freq, &min, &max);
                        if (freq != 0)printf("%"PRIu64",%d,%d,555,%d,%f,%f,%f,%d,%d\n", slot->slot_stop_time,
                                 (*(Global_Sources.array+k)), (*(Global_Destinations.array+j)), freq, avg, stdev,avg_dev, min, max);
                        freq = 0;
                        avg = 0.0;
                        i = 0;
                        free(val_array);
                        j++;
                }
                j = 0;
                k++;
        }
        Global_Sources.n = 0;
        Global_Destinations.n = 0;
        i = 0;
}
void process_wifi_mon(SLOT *slot){
	unsigned int i = 0;
	unsigned int k = 0;
	unsigned int j = 0;
	unsigned int freq = 0;
	double avg = 0.0;
	double stdev = 0.0;
	double avg_dev = 0.0;
	FRAME *_frame = slot->frame_array;
	while(k < Global_Sources.n){
		while(j < Global_Destinations.n){
			_frame = slot->frame_array;
			unsigned int *val_array = (unsigned int*)calloc(slot->n, sizeof(int)); // worst case scenario
			while(i < slot->n){
				WiFi_Frame *frm = (WiFi_Frame*)_frame->frame_ptr;
				if ((*(Global_Sources.array+k) == frm->src_id) && (*(Global_Destinations.array+j) == frm->dst_id)){
					val_array[freq] = frm->frame_length;
					freq++;
					avg += frm->frame_length;
				}
				_frame = _frame->next;
				i++;
			}
			double test = avg/freq;
			avg = _math_average(val_array, freq);
			stdev = _math_stdev(_math_variance(val_array, avg, freq));
			avg_dev = _math_avg_dev(val_array, freq);
			if (isnan(avg_dev)) avg_dev = 0.0;
			if (isnan(stdev)) stdev = 0.0;
			unsigned int min = 0;
			unsigned int max = 0;
			_math_minmax(val_array, freq, &min, &max);
			if (freq != 0)printf("%"PRIu64",%d,%d,555,%d,%f,%f,%f,%d,%d\n", slot->slot_stop_time,
				(*(Global_Sources.array+k)), (*(Global_Destinations.array+j)), freq, avg, stdev,avg_dev, min, max);
			freq = 0;
			avg = 0.0;
			i = 0;
			free(val_array);
			j++;
		}
		j = 0;
		k++;
	}
	Global_Sources.n = 0;
	Global_Destinations.n = 0;
	i = 0;
}
void process_ip_short(SLOT *slot){
        unsigned int i = 0;
        unsigned int k = 0;
        unsigned int j = 0;
        unsigned int freq = 0;
        double avg = 0.0;
        double stdev = 0.0;
        double avg_dev = 0.0;
        FRAME *_frame = slot->frame_array;
        while (k < Global_Sources.n){   // for every source
                while(j < Global_Destinations.n){ // for every destination
                        _frame = slot->frame_array;
                        unsigned int *val_array = (unsigned int*)calloc(slot->n, sizeof(int)); // worst$
                        while(i < slot->n){ // for each packet
                                IP_Frame *frm = (IP_Frame*)_frame->frame_ptr;
                                if ((*(Global_Sources.array+k) == frm->src_id) && (*(Global_Destinations.array+j) == frm->dst_id)){
                                        val_array[freq] = frm->packet_size;
                                        freq++;
                                        avg += frm->packet_size;
                                }
                                _frame = _frame->next;
                                i++;
                        }
                        double test = avg/freq;
                        avg = _math_average(val_array, freq);
                        stdev = _math_stdev(_math_variance(val_array, avg, freq));
                        avg_dev = _math_avg_dev(val_array, freq);
                        if (isnan(avg_dev)) avg_dev=0.0;
                        if (isnan(stdev)) stdev = 0.0;
                        unsigned int min = 0;
                        unsigned int max = 0;
			_math_minmax(val_array, freq, &min, &max);
                        if (freq != 0)printf("%"PRIu64",%d,%d,555,%d,%f,%f,%f,%d,%d\n", slot->slot_stop_time,
                                 (*(Global_Sources.array+k)), (*(Global_Destinations.array+j)), freq, avg, stdev,avg_dev, min, max);
                        freq = 0;
                        avg = 0.0;
                        i = 0;
                        free(val_array);
                        j++;
                }
                j = 0;
                k++;
        }
        Global_Sources.n = 0;
        Global_Destinations.n = 0;
        i = 0;
}
void process_zigbee(SLOT *slot){
        unsigned int i = 0;
	unsigned int k = 0;
	unsigned int j = 0;
	unsigned int freq = 0;
	double avg = 0.0;
	double stdev = 0.0;
	double avg_dev = 0.0;
        FRAME *_frame = slot->frame_array;
	while (k < Global_Sources.n){	// for every source
		while(j < Global_Destinations.n){ // for every destination
			_frame = slot->frame_array;
			unsigned int *val_array = (unsigned int*)calloc(slot->n, sizeof(int)); // worst case scenario
			while(i < slot->n){ // for each packet
				ZigBee_Frame *frm = (ZigBee_Frame*)_frame->frame_ptr;
				if ((*(Global_Sources.array+k) == frm->src_id) && (*(Global_Destinations.array+j) == frm->dst_id)){
					val_array[freq] = frm->packet_size;
					freq++;
					avg += frm->packet_size;
				}
				_frame = _frame->next;
				i++;
			}
			double test = avg/freq;
			avg = _math_average(val_array, freq);
			stdev = _math_stdev(_math_variance(val_array, avg, freq));
			avg_dev = _math_avg_dev(val_array, freq);
			if (isnan(avg_dev)) avg_dev=0.0;
			if (isnan(stdev)) stdev = 0.0;
			unsigned int min = 0;
			unsigned int max = 0;
			_math_minmax(val_array, freq, &min, &max);
			if (freq != 0)printf("%"PRIu64",%04x,%04x,555,%d,%f,%f,%f,%d,%d\n", slot->slot_stop_time,
				 (*(Global_Sources.array+k)), (*(Global_Destinations.array+j)), freq, avg, stdev,avg_dev, min, max);
			freq = 0;
			avg = 0.0;
			i = 0;
			free(val_array);
			j++;
		}
		j = 0;
		k++;
	}
	Global_Sources.n = 0;
	Global_Destinations.n = 0;
        i = 0;
}

int main(int argc, char **argv){ int mode = 1;
	FILE *file = fopen(argv[1], "rb");
	if (argc > 2){
		mode = atoi(argv[2]);
	}
	if (argc > 3){

	}
	int version = 0;
	int pkt_number = 0;
	unsigned char slot_index;
	fseek(file, 0, SEEK_SET);
	fread(&version, sizeof(int), 1, file);
	fread(&pkt_number,sizeof(int), 1,file);
//	printf("Version: %d Packets: %d\n", version, pkt_number);
	SLOT *slot;
	slot = slot_init();
	while(pkt_number--){
		int t_len = 0;
		if (mode == 0){	// Wifi
			WiFi_Frame t_wf_frm;
			fread(&t_len, sizeof(char), 1, file);
			fread(&t_wf_frm, t_len, 1, file);
			if (slot->n == 0){
				if (global_start_time == 0) global_start_time = (t_wf_frm.timestamp/1000000)*1000000;
				update_stop_start_times(slot);
				frame_add(slot, &t_wf_frm, t_len, 1);
				continue;
			} else {
				if ((t_wf_frm.timestamp >= slot->slot_start_time)&&(t_wf_frm.timestamp <= slot->slot_stop_time)){
					frame_add(slot, &t_wf_frm, t_len, 1);
				} else {
					get_unique_number_wi(slot);
					// process
					process_wifi_mon(slot);
					free_slot(slot);
					update_stop_start_times(slot);
					if ((t_wf_frm.timestamp > slot->slot_start_time) && (t_wf_frm.timestamp < slot->slot_stop_time)){
						 while(t_wf_frm.timestamp < slot->slot_start_time){
							// if 0's neede, can be padded here
							update_stop_start_times(slot);
						}
					}
					frame_add(slot, &t_wf_frm, t_len, 1);
					free(Global_Sources.array);
					free(Global_Destinations.array);
					continue;
				}
				continue;
			}
			printf("%"PRIu64",%04x,%04x,%d,%d,%d\n",
				t_wf_frm.timestamp, t_wf_frm.src_id, t_wf_frm.dst_id,
				t_wf_frm.bssid_len, t_wf_frm.essid_len, t_wf_frm.frame_type);
		} else if (mode == 1){	// ZigBee
			ZigBee_Frame t_zb_frm;
        	        fread(&t_len, sizeof(char), 1, file);
        	        fread(&t_zb_frm, t_len, 1, file);
			if (slot->n == 0){	// initial start, the rest will be done later in else
				if (global_start_time == 0) global_start_time = (t_zb_frm.timestamp/1000000)*1000000;
				update_stop_start_times(slot);
//				printf("*%"PRIu64"-%"PRIu64" Time: %"PRIu64"\n", slot->slot_start_time, slot->slot_stop_time, t_zb_frm.timestamp);
//				if (t_zb_frm.timestamp > slot->slot_stop_time){
//					printf("\nMoney\n");
//					process_zigbee(slot);
					// processing needs to be done here
					// free slot
					// identify whether current frame fits the slot
					// if not identify how many INTERVALS doesnt fit
						// output 0 until current frame fits and update slot
//				}
				//slot->frame_array = (FRAME*)calloc(1, sizeof(FRAME));
				//slot->frame_array->frame_len = t_len;
				//slot->frame_array->frame_ptr = (char*)calloc(1, t_len);
				//memcpy(slot->frame_array->frame_ptr, &t_zb_frm, t_len);
				//slot->n++;
				frame_add(slot, &t_zb_frm, t_len, 1);
				continue;
			} else {
				if ((t_zb_frm.timestamp >= slot->slot_start_time)&&(t_zb_frm.timestamp <= slot->slot_stop_time)){
				//	FRAME *frame = (FRAME*)calloc(1, sizeof(FRAME));
			//		FRAME *t_frame = slot->frame_array;
			//		while(t_frame->next != 0) t_frame = t_frame->next;
					// I have the last frame
			//		t_frame->next = frame;
			//		frame->frame_len = t_len;
			//		frame->frame_ptr = (char*)calloc(1, t_len);
			//		memcpy(frame->frame_ptr, &t_zb_frm, t_len);
			//		slot->n++;
					
//					printf(" %"PRIu64"-%"PRIu64" Time: %"PRIu64"\n", slot->slot_start_time, slot->slot_stop_time, t_zb_frm.timestamp);
					frame_add(slot, &t_zb_frm, t_len, 1);
				} else { // the timestamp is higher
					// do stat analysis
//					printf("%"PRIu64"-%"PRIu64" Time: %"PRIu64"\n", slot->slot_start_time, slot->slot_stop_time, t_zb_frm.timestamp);
					
					get_unique_number(slot);
					process_zigbee(slot);
					//printf("\ngot a slot, pkt: %d\n", slot->n);
					if (t_zb_frm.timestamp > slot->slot_stop_time){
						process_zigbee(slot);
					}
					free_slot(slot);
//					printf("\nMallocs: %d Frees: %d\n", mallocs, frees);
//					mallocs = 0; frees =0;
					update_stop_start_times(slot);
					if ((t_zb_frm.timestamp > slot->slot_start_time) && (t_zb_frm.timestamp < slot->slot_stop_time)){
						while(t_zb_frm.timestamp < slot->slot_start_time){
							// zeros can be padded here
							//for now catchup
							update_stop_start_times(slot);
						}
					}
					frame_add(slot, &t_zb_frm, t_len, 1); // add current handled frame
					free(Global_Sources.array);
					free(Global_Destinations.array);
//					free_slot(slot);
	//				return 0;
					// free slot

					// re-initialize
				}
				continue;
			}
	// if (t_zb_frm.frame_type == 0) continue;
	// printf("%04x\n",t_zb_frm.src_id);
	          //      printf("%"PRIu64",%04x,%04x,%d,%d,%d\n",
        	//                t_zb_frm.timestamp, t_zb_frm.src_id, t_zb_frm.dst_id,
               // 	        t_zb_frm.frame_type, t_zb_frm.packet_size, t_zb_frm.flags);
	// if (t_zb_frm.flags != 1) inc_relation(Start, t_zb_frm.src_id, t_zb_frm.dst_id);
		} else if (mode == 2){	// IP Short
			IP_Frame t_ips_frm;
        	        fread(&t_len, sizeof(char), 1, file);
	                fread(&t_ips_frm, sizeof(IP_Frame), 1, file);
			if (slot->n == 0){      // initial start, the rest will be done later in else
                                if (global_start_time == 0) global_start_time = (t_ips_frm.timestamp/1000000)*1000000;
                                update_stop_start_times(slot);
//                              printf("*%"PRIu64"-%"PRIu64" Time: %"PRIu64"\n", slot->slot_start_time, slot->slot_stop_time, t_zb_frm.timestamp);
//                              if (t_zb_frm.timestamp > slot->slot_stop_time){
//                                      printf("\nMoney\n");
//                                      process_zigbee(slot);
                                        // processing needs to be done here
                                        // free slot
                                        // identify whether current frame fits the slot
                                        // if not identify how many INTERVALS doesnt fit
                                                // output 0 until current frame fits and update slot
//                              }
                                //slot->frame_array = (FRAME*)calloc(1, sizeof(FRAME));
                                //slot->frame_array->frame_len = t_len;
                                //slot->frame_array->frame_ptr = (char*)calloc(1, t_len);
                                //memcpy(slot->frame_array->frame_ptr, &t_zb_frm, t_len);
                                //slot->n++;
                                frame_add(slot, &t_ips_frm, t_len, 1);
                                continue;
                        } else {
                                if ((t_ips_frm.timestamp >= slot->slot_start_time)&&(t_ips_frm.timestamp <= slot->slot_stop_time)){
					frame_add(slot, &t_ips_frm, t_len, 1);
                                } else { // the timestamp is higher
                                        // do stat analysis
//                                      printf("%"PRIu64"-%"PRIu64" Time: %"PRIu64"\n", slot->slot_start_time, slot->slot_stop_time, t_zb_frm.timestamp);

                                        get_unique_number(slot);
                                        process_ip_short(slot);
                                        //printf("\ngot a slot, pkt: %d\n", slot->n);
                                        if (t_ips_frm.timestamp > slot->slot_stop_time){
                                                process_ip_short(slot);
                                        }
                                        free_slot(slot);
//                                      printf("\nMallocs: %d Frees: %d\n", mallocs, frees);
//                                      mallocs = 0; frees =0;
                                        update_stop_start_times(slot);
                                        if ((t_ips_frm.timestamp > slot->slot_start_time) && (t_ips_frm.timestamp < slot->slot_stop_time)){
                                                while(t_ips_frm.timestamp < slot->slot_start_time){
                                                        // zeros can be padded here
                                                        //for now catchup
                                                        update_stop_start_times(slot);
                                                }
                                        }
                                        frame_add(slot, &t_ips_frm, t_len, 1); // add current handled frame
                                        free(Global_Sources.array);
                                        free(Global_Destinations.array);
//                                      free_slot(slot);
        //                              return 0;
                                        // free slot

                                        // re-initialize
                                }
                                continue;
                        }
	// if (t_zb_frm.frame_type == 0) continue;
	// printf("%04x\n",t_zb_frm.src_id);
//	                printf("%"PRIu64",%04x,%04x,%d,%d\n",
//	                        t_ips_frm.timestamp, t_ips_frm.src_id, t_ips_frm.dst_id,
//	                        t_ips_frm.protocol, t_ips_frm.packet_size);//, t_zb_frm.flags);
	// if (t_zb_frm.flags != 1) inc_relation(Start, t_zb_frm.src_id, t_zb_frm.dst_id);
		} else if (mode == 3){	// Sound else {
			Audio_Frame t_snd_frm;
                        fread(&t_len, sizeof(char), 1, file);
                        fread(&t_snd_frm, t_len, 1, file);
			if (slot->n == 0){      // initial start, the rest will be done later in else
                                if (global_start_time == 0) global_start_time = (t_snd_frm.timestamp/1000000)*1000000;
                                update_stop_start_times(slot);
				frame_add(slot, &t_snd_frm, t_len, 1);
			} else {
				if ((t_snd_frm.timestamp >= slot->slot_start_time)&&(t_snd_frm.timestamp <= slot->slot_stop_time)){
					frame_add(slot, &t_snd_frm, t_len, 1);
					continue;
				} else {
					process_audio(slot);
					free_slot(slot);
					update_stop_start_times(slot);
                                        if ((t_snd_frm.timestamp > slot->slot_start_time) && (t_snd_frm.timestamp < slot->slot_stop_time)){
                                                while(t_snd_frm.timestamp < slot->slot_start_time){
                                                        // zeros can be padded here
                                                        //for now catchup
                                                        update_stop_start_times(slot);
                                                }
                                        }
					frame_add(slot, &t_snd_frm, t_len, 1);
					continue;
				}
			}
			//printf("Mode not supported\n");
		}
	}
// Relation *ptr = Start; // while(ptr->next != 0){ // printf("Relation: %04x -> %04x. Hits: %lu\n", ptr->src_id, ptr->dst_id, ptr->counter); // ptr = ptr->next; //	} //	printf("Debug break");
	return 0;
}
