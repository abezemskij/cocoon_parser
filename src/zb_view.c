#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
//#include <fstream>

typedef struct ZigBee_Frame{
        uint64_t timestamp;
        unsigned short src_id;
        unsigned short dst_id;
        unsigned char frame_type;
        unsigned char packet_size;
        unsigned char flags; // 1 Bad_Checksum
}ZigBee_Frame;

int  main(){
	FILE *zb_stream =stdin;
	int zb_msg_len = sizeof(ZigBee_Frame);
	char zb_msg_buf[zb_msg_len];
	unsigned char buf_index =0;
	// do EOF handler
	char *P_EOF = (char*)calloc(1, zb_msg_len);
	memset(P_EOF, '\xFF', zb_msg_len);
	//zb_file = fopen("test.csv", "rb+");
	while(1){
		zb_msg_buf[buf_index++] = fgetc(zb_stream); // get char from zigbee stream
		if (buf_index == zb_msg_len){
			// check for personal EOF
			if (memcmp(zb_msg_buf, P_EOF, zb_msg_len) == 0){
				 return 0;
			}
			// process
			ZigBee_Frame *zb_msg = (ZigBee_Frame*)calloc(1, sizeof(ZigBee_Frame));
			zb_msg = (ZigBee_Frame*)&zb_msg_buf[1];
			printf("%" PRIu64 ",%04x,%04x,%d,%d,%d\n", zb_msg->timestamp, zb_msg->src_id, zb_msg->dst_id, zb_msg->packet_size, zb_msg->frame_type, zb_msg->flags);
			fflush(stdout);
			buf_index = 0;
		}
	}
	return 0;
}
