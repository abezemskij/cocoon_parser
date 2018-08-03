#include "slot_wrap.h" 

void free_slot_frame_type(SLOT *slot, unsigned char type){
	// wifi_struct_internal = 1
	unsigned int i = 0;
	unsigned int k = 0;
	FRAME *ptr = slot->frame_array;
	switch(type){
		case 1:
			while(i < slot->n){
				ptr = slot->frame_array;
				while(k < i){ ptr = ptr->next; k++; }
				k = 0;
				if (ptr == 0){ printf("free_slot_frame_type func failed, abnormal condition\n"); break;}
//				free( ((wifi_struct_internal*)(ptr->frame_ptr))->src_mac );
//				free( ((wifi_struct_internal*)(ptr->frame_ptr))->dst_mac );
				i++;
			}
			break;
	}
}

void analyse_slot_add(SLOT *slot, void *object, unsigned char object_size, unsigned char type, Enum_Type *Enumerator){
	uint64_t time = *((uint64_t*)object);
	if (slot->n == 0){ slot->slot_start_time = time; slot->slot_stop_time = time+1000000; }//ms
	if (time > slot->slot_stop_time){ // if current time of an object is higher then stop time i.e. exceeds
		// process current slot
		GLOBAL_KNOWLEDGE *_glob;// = perform_global_features(slot, 1); // 1 wifi
		switch(type){
			case 1: //wifi
				_glob = perform_global_features(slot, type);
				cpu_wifi_out(slot, _glob, Enumerator);

				break;
			case 2: // ip
				_glob = perform_global_features(slot, type);
				cpu_ip_out(slot, _glob, Enumerator);
				
				break;
			case 3: // zigbee
				_glob = perform_global_features(slot, type);
				cpu_zbee_out(slot, _glob, Enumerator);
				break;
			default:
				printf("Defaulted in analyse slot_add (slot_wrap.c)");
				break;
		}
		
		global_knowledge_free(_glob);
		// free the slot, taking into account types
//		free_slot_frame_type(slot, type);
		free_slot(slot);

		// set current object as 
		frame_add(slot, object, object_size, 1); // type needs to be 1, can be removed later
		slot->slot_start_time = slot->slot_stop_time;
		slot->slot_stop_time = slot->slot_start_time +1000000;

	} else if (time >= slot->slot_start_time){ // within the range start <-> stop times, good
		frame_add(slot, object, object_size, 1);
	} else {	// something went wrong for sure
		printf("analyse_slot_add func failed, abnormal if condition! \n");
	}

}
unsigned short *add_short_to_array(unsigned short *array, unsigned short val, unsigned int index){
	unsigned short *_temp_array = array;
        unsigned short *_new_array = (unsigned short*)calloc(index, sizeof(short));
        memcpy(_new_array, _temp_array, sizeof(short)*(index-1));
        free(_temp_array);
        _new_array[index-1] = val;
	return _new_array;
}
GLOBAL_KNOWLEDGE *perform_global_features(SLOT *slot, unsigned char type){
	GLOBAL_KNOWLEDGE *_glob = global_knowledge_init();
	unsigned int i = 0;
	unsigned int unique_type = 0;
	unsigned int unique_subtype = 0;
	unsigned int unique_exttype = 0;
	unsigned int unique_src = 0;
	unsigned int unique_dst = 0;

	unsigned short temp = 0;
	unsigned short *type_array;
	unsigned short *subtype_array;
	unsigned short *exttype_array;
	unsigned short *src_array;
	unsigned short *dst_array;

	FRAME *_frame = slot->frame_array;

	while(i < slot->n){ // go through all frames

		switch(type){
			case 1:		// wifi struct internal
				if (_frame != 0){
					if (unique_type == 0){	// first value
						type_array = (unsigned short*)calloc(1, sizeof(short));
						*type_array = ((wifi_struct_internal*)_frame->frame_ptr)->type;
						unique_type++;
					} else {
						unsigned int k = 0;
						unsigned char found = 0;
						while(k < unique_type){	// cycle through all unique types
							if(type_array[k] == ((wifi_struct_internal*)_frame->frame_ptr)->type){
								found = 1;
								break;
							}
							k++;
						}
						if (found != 1){	// if not found then add
							unique_type++;
							type_array = add_short_to_array(type_array, ((wifi_struct_internal*)_frame->frame_ptr)->type, unique_type);
						}
					}
					if (unique_subtype == 0){
						subtype_array = (unsigned short*)calloc(1, sizeof(short));
						*subtype_array = ((wifi_struct_internal*)_frame->frame_ptr)->subtype;
						unique_subtype++;
					} else {
						unsigned int k = 0;
						unsigned char found = 0;
						while(k < unique_subtype){	// cycle through all unique types
							if(subtype_array[k] == ((wifi_struct_internal*)_frame->frame_ptr)->subtype){
								found = 1;
								break;
							}
							k++;
						}
						if (found != 1){	// if not found then add
							unique_subtype++;
							subtype_array = add_short_to_array(subtype_array, ((wifi_struct_internal*)_frame->frame_ptr)->subtype, unique_subtype);
						}
					}
					if (unique_src == 0){
						src_array = (unsigned short*)calloc(1,sizeof(short));
						*src_array = ((wifi_struct_internal*)_frame->frame_ptr)->src_mac;
						unique_src++;
					} else {
						unsigned int k = 0;
						unsigned char found = 0;
						while(k < unique_src){	// cycle through all unique types
							if(src_array[k] == ((wifi_struct_internal*)_frame->frame_ptr)->src_mac){
								found = 1;
								break;
							}
							k++;
						}
						if (found != 1){	// if not found then add
							unique_src++;
							src_array = add_short_to_array(src_array, ((wifi_struct_internal*)_frame->frame_ptr)->src_mac, unique_src);
						}
					}
					if (unique_dst == 0){
						dst_array = (unsigned short*)calloc(1,sizeof(short));
						*dst_array= ((wifi_struct_internal*)_frame->frame_ptr)->dst_mac;
						unique_dst++;
					} else {
						unsigned int k = 0;
						unsigned char found = 0;
						while(k < unique_dst){	// cycle through all unique types
							if(dst_array[k] == ((wifi_struct_internal*)_frame->frame_ptr)->dst_mac){
								found = 1;
								break;
							}
							k++;
						}
						if (found != 1){	// if not found then add
							unique_dst++;
							dst_array = add_short_to_array(dst_array, ((wifi_struct_internal*)_frame->frame_ptr)->dst_mac, unique_dst); //_new_array;
						}
					}
				}
				break;
			case 2:		// IP struct internal
				if (_frame != 0){
					if (unique_type == 0){	// first value	// proto
						type_array = (unsigned short*)calloc(1, sizeof(short));
						*type_array = ((ip_struct_internal*)_frame->frame_ptr)->protocol;
						unique_type++;
					} else {
						unsigned int k = 0;
						unsigned char found = 0;
						while(k < unique_type){	// cycle through all unique types
							if(type_array[k] == ((ip_struct_internal*)_frame->frame_ptr)->protocol){
								found = 1;
								break;
							}
							k++;
						}
						if (found != 1){	// if not found then add
							unique_type++;
							type_array = add_short_to_array(type_array, ((ip_struct_internal*)_frame->frame_ptr)->protocol, unique_type);
						}
					}
					if (unique_subtype == 0){	// src_port
						subtype_array = (unsigned short*)calloc(1,sizeof(short));
						*subtype_array= ((ip_struct_internal*)_frame->frame_ptr)->src_port;
						unique_subtype++;
					} else {
						unsigned int k = 0;
						unsigned char found = 0;
						while(k < unique_subtype){	// cycle through all unique types
							if(subtype_array[k] == ((ip_struct_internal*)_frame->frame_ptr)->src_port){
								found = 1;
								break;
							}
							k++;
						}
						if (found != 1){	// if not found then add
							unique_subtype++;
							subtype_array = add_short_to_array(subtype_array, ((ip_struct_internal*)_frame->frame_ptr)->src_port, unique_subtype); //_new_array;
						}
					}
					if (unique_exttype == 0){ 	// dst port
						exttype_array = (unsigned short*)calloc(1,sizeof(short));
						*exttype_array= ((ip_struct_internal*)_frame->frame_ptr)->dst_port;
						unique_exttype++;
					} else {
						unsigned int k = 0;
						unsigned char found = 0;
						while(k < unique_exttype){	// cycle through all unique types
							if(exttype_array[k] == ((ip_struct_internal*)_frame->frame_ptr)->dst_port){
								found = 1;
								break;
							}
							k++;
						}
						if (found != 1){	// if not found then add
							unique_exttype++;
							exttype_array = add_short_to_array(exttype_array, ((ip_struct_internal*)_frame->frame_ptr)->dst_port, unique_exttype); //_new_array;
						}
					}
					if (unique_src == 0){
						src_array = (unsigned short*)calloc(1,sizeof(short));
						*src_array = ((ip_struct_internal*)_frame->frame_ptr)->src_ip;
						unique_src++;
					} else {
						unsigned int k = 0;
						unsigned char found = 0;
						while(k < unique_src){	// cycle through all unique types
							if(src_array[k] == ((ip_struct_internal*)_frame->frame_ptr)->src_ip){
								found = 1;
								break;
							}
							k++;
						}
						if (found != 1){	// if not found then add
							unique_src++;
							src_array = add_short_to_array(src_array, ((ip_struct_internal*)_frame->frame_ptr)->src_ip, unique_src);
						}
					}
					if (unique_dst == 0){
						dst_array = (unsigned short*)calloc(1,sizeof(short));
						*dst_array= ((ip_struct_internal*)_frame->frame_ptr)->dst_ip;
						unique_dst++;
					} else {
						unsigned int k = 0;
						unsigned char found = 0;
						while(k < unique_dst){	// cycle through all unique types
							if(dst_array[k] == ((ip_struct_internal*)_frame->frame_ptr)->dst_ip){
								found = 1;
								break;
							}
							k++;
						}
						if (found != 1){	// if not found then add
							unique_dst++;
							dst_array = add_short_to_array(dst_array, ((ip_struct_internal*)_frame->frame_ptr)->dst_ip, unique_dst); //_new_array;
						}
					}
				}
				
				break;
			case 3:		// ZBee
				if (_frame != 0){
					if (unique_type == 0){	// first value
						type_array = (unsigned short*)calloc(1, sizeof(short));
						*type_array = ((zbee_struct_internal*)_frame->frame_ptr)->pkt_type;
						unique_type++;
					} else {
						unsigned int k = 0;
						unsigned char found = 0;
						while(k < unique_type){	// cycle through all unique types
							if(type_array[k] == ((zbee_struct_internal*)_frame->frame_ptr)->pkt_type){
								found = 1;
								break;
							}
							k++;
						}
						if (found != 1){	// if not found then add
							unique_type++;
							type_array = add_short_to_array(type_array, ((zbee_struct_internal*)_frame->frame_ptr)->pkt_type, unique_type);
						}
					}
					if (unique_src == 0){
						src_array = (unsigned short*)calloc(1,sizeof(short));
						*src_array = ((zbee_struct_internal*)_frame->frame_ptr)->src_id;
						unique_src++;
					} else {
						unsigned int k = 0;
						unsigned char found = 0;
						while(k < unique_src){	// cycle through all unique types
							if(src_array[k] == ((zbee_struct_internal*)_frame->frame_ptr)->src_id){
								found = 1;
								break;
							}
							k++;
						}
						if (found != 1){	// if not found then add
							unique_src++;
							src_array = add_short_to_array(src_array, ((zbee_struct_internal*)_frame->frame_ptr)->src_id, unique_src);
						}
					}
					if (unique_dst == 0){
						dst_array = (unsigned short*)calloc(1,sizeof(short));
						*dst_array= ((zbee_struct_internal*)_frame->frame_ptr)->dst_id;
						unique_dst++;
					} else {
						unsigned int k = 0;
						unsigned char found = 0;
						while(k < unique_dst){	// cycle through all unique types
							if(dst_array[k] == ((zbee_struct_internal*)_frame->frame_ptr)->dst_id){
								found = 1;
								break;
							}
							k++;
						}
						if (found != 1){	// if not found then add
							unique_dst++;
							dst_array = add_short_to_array(dst_array, ((zbee_struct_internal*)_frame->frame_ptr)->dst_id, unique_dst); //_new_array;
						}
					}
				}
				break;
			default:
			
				break;
		}

		_frame = _frame->next;
		i++;
	}
	_glob->Global_Types->n = unique_type;
	_glob->Global_SubTypes->n = unique_subtype;
	_glob->Global_ExtTypes->n = unique_exttype;
	_glob->Global_Sources->n = unique_src;
	_glob->Global_Destinations->n = unique_dst;

        _glob->Global_Types->array = type_array;
        _glob->Global_SubTypes->array = subtype_array;
	_glob->Global_ExtTypes->array = exttype_array;
        _glob->Global_Sources->array = src_array;
        _glob->Global_Destinations->array = dst_array;

	return _glob;
}
