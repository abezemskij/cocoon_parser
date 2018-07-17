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

void analyse_slot_add(SLOT *slot, void *object, unsigned char object_size, unsigned char type){
	uint64_t time = *((uint64_t*)object);
	if (slot->n == 0){ slot->slot_start_time = time; slot->slot_stop_time = time+1000000; }//ms
	if (time > slot->slot_stop_time){ // if current time of an object is higher then stop time i.e. exceeds
		// process current slot

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

GLOBAL_KNOWLEDGE *perform_global_features(SLOT *slot, unsigned char type){
	GLOBAL_KNOWLEDGE *_glob = global_knowledge_init();
	unsigned int i = 0;
	unsigned int unique_type = 0;
	unsigned int unique_subtype = 0;
	unsigned int unique_src = 0;
	unsigned int unique_dst = 0;

	unsigned short temp = 0;
	unsigned short *type_array;
	unsigned short *subtype_array;
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
						while(k < unique_type){
							if(type_array[k] == ((wifi_struct_internal*)_frame->frame_ptr)->type){
								found = 1;
								break;
							}
							k++;
						}
						if (found != 1);
					}
					if (unique_subtype == 0){
						subtype_array = (unsigned short*)calloc(1, sizeof(short));
						*subtype_array = ((wifi_struct_internal*)_frame->frame_ptr)->subtype;
						unique_subtype++;
					} else {

					}
					if (unique_src == 0){
						src_array = (unsigned short*)calloc(1,sizeof(short));
						*src_array = ((wifi_struct_internal*)_frame->frame_ptr)->src_mac;
						unique_src++;
					} else {

					}
					if (unique_dst == 0){
						dst_array = (unsigned short*)calloc(1,sizeof(short));
						*dst_array= ((wifi_struct_internal*)_frame->frame_ptr)->dst_mac;
					} else {

					}
				}
				break;
			case 2:		// IP struct internal
				
				break;
			case 3:		// ZBee
				
				break;
		}

		_frame = _frame->next;
		i++;
	}

	return _glob;
}
