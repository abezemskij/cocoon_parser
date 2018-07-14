#include "int_zbee.h"

void ieee_man_frames_handler(char *after_length_ptr, ZigBee_Frame *zb_object, Enum_Type *Enumerator){
        char *t_ptr = after_length_ptr;
        while(*t_ptr != '\0'){
                if (*t_ptr++ == ','){
                        t_ptr -=1; // compensate space
                        // get the value from after_length_ptr to t_ptr
                        zb_object->frame_type = manage_enumerations(Enumerator, after_length_ptr, t_ptr);
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
                        zb_object->frame_type = manage_enumerations(Enumerator, after_length_ptr, t_ptr);;
                        break;
                }
        }
}

void zigb_man_frames_handler(char *after_length_ptr, ZigBee_Frame *zb_object, Enum_Type *Enumerator){
        char *t_ptr = after_length_ptr;
        while(*t_ptr != '\0'){
                if (*t_ptr++ == ','){
                        t_ptr -=1; // compensate space
                        // get the value from after_length_ptr to t_ptr
                        zb_object->frame_type = manage_enumerations(Enumerator, after_length_ptr, t_ptr);
                        // compensate ", "
                        t_ptr +=2;
                        //reusing all local variables
                        if (!(zb_object->flags & 1))zb_object->flags |= manage_comparison(t_ptr, '\0', "Bad FCS");
                        break;
                }
                if (*t_ptr == '\0'){
                        zb_object->frame_type = manage_enumerations(Enumerator, after_length_ptr, t_ptr);
                        break;
                }
        }
}

unsigned int exctract_source_id(char *line, ZigBee_Frame *zb_object, Enum_Type *Enumerator){
        // use int to store src and dst
        unsigned short src = 0;
        unsigned short dst = 0;
        unsigned int srcdst= 0;
        char *t_ptr = line;

        while(*t_ptr++ != ' '); // skip the number
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
                                ieee_man_frames_handler(t_ptr, zb_object, Enumerator);
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
                                zigb_man_frames_handler(t_ptr, zb_object, Enumerator);
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
                        if (*t_ptr == 'Z'){t_ptr=t_ptr-3; *t_ptr='B'; t_ptr--; break; } //  17:51:57.945093901       0x0000 →            ZigBee 42 Beacon, S... case
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
                                ieee_man_frames_handler(t_ptr, zb_object, Enumerator);
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
                                zigb_man_frames_handler(t_ptr, zb_object, Enumerator);
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
        ZigBee_Frame *ret_ptr = (ZigBee_Frame*)calloc(1, sizeof(ZigBee_Frame));
        ret_ptr->timestamp = convert_date_to_epoch(line);
        ret_ptr->src_id = 0x0000;
        ret_ptr->dst_id = 0x0000;
        ret_ptr->packet_size = 0;
        ret_ptr->frame_type = 0;
        ret_ptr->flags = 0;

        return ret_ptr;
}

ZigBee_Frame **process_zigbee_lines(char *ptr, unsigned long lines, unsigned int *filtered, Enum_Type *Enumerator){
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
                //              printf("\nline==0x00\n");
                                free(line);
                                l_count++;
                                line = t_upd[0];
                                break;
                        }
                //      printf("line=t_upd\n");
                        free(line); l_count++;
                        line = t_upd[0];
                        continue;
                }
        //      printf("\nValidation went fine");
                zb_arr[zb_arr_i] = (ZigBee_Frame*)calloc(1, sizeof(ZigBee_Frame));
                zb_arr[zb_arr_i]->timestamp = convert_date_to_epoch(line);
                exctract_source_id(line, zb_arr[zb_arr_i], Enumerator);
                free(line);
                line = t_upd[0];
                zb_arr_i++;
                l_count++;
        }
        *filtered = --zb_arr_i;
        free(t_upd);
        return zb_arr;
}

void process_zigbee_file_input_live(unsigned char live_mode, char *line_buffer, char *out_filename_ptr, unsigned short argument_flags, char *args, Enum_Type *Enumerator){
        unsigned int line_count = 0;
        unsigned int filtered = 0;
	int live_descriptor_write = 0; // Find a solution
        ZigBee_Frame **zb_arr_ptr = process_zigbee_lines(line_buffer, 1, &filtered, Enumerator);
        if (filtered != 0xFFFFFFFF) write_out_frames_new((void*)zb_arr_ptr, 1,'\0', line_count, out_filename_ptr, argument_flags, args);
        fflush(stdout);
        if ((filtered != 0xFFFFFFFF) && (live_descriptor_write)){write_descriptor(Enumerator, "zbee_protocols.csv", out_filename_ptr, argument_flags, args); live_descriptor_write = 0;}
}
