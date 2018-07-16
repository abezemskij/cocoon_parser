#include "int_ipshort.h"

void process_ip_frame(char *line, IP_Frame *ip_frm, Enum_Type *Enumerator_Addr, Enum_Type *Enumerator_Proto){
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
                ip_frm->src_ip = enum_find_frame_type(source_ip, Enumerator_Addr);
                if (ip_frm->src_ip == 0xFFFF){
                        ip_frm->src_ip = enum_add(source_ip, Enumerator_Addr);
                }
                 ip_frm->dst_ip = enum_find_frame_type(destin_ip, Enumerator_Addr);
                if (ip_frm->dst_ip == 0xFFFF){
                        ip_frm->dst_ip = enum_add(destin_ip, Enumerator_Addr);
                }
                free(source_ip);
                free(destin_ip);
                t_ptr += i+1;
                t_ptr = return_me_not_char(t_ptr, ' ');
                // Proto extract
//              if (*t_ptr == ' ') t_ptr = return_me_not_char(t_ptr, ' ');
                i = 0;
                src_ip = t_ptr;
                dst_ip = t_ptr;
                while(*dst_ip++ != ' ') i++;
                source_ip = (char*)calloc(1, i+1);
                memcpy(source_ip, src_ip, i); // copied the proto
                ip_frm->protocol = enum_find_frame_type(source_ip, Enumerator_Proto);
                if (ip_frm->protocol == 0xFFFF){        // shift towards short, 0xFF is 255 protos, easily achieved.
                        ip_frm->protocol = enum_add(source_ip, Enumerator_Proto);
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
//              while(t_ptr++ != '\0');
        }
}

IP_Frame **process_ip_frame_lines(char *ptr, unsigned long lines, unsigned int *filtered, Enum_Type *Enumerator_Addr, Enum_Type *Enumerator_Proto){
        char *line = ptr;
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
                                line = t_upd[0]; 
                                break;
                        }
                        free(line); l_count++;
                        line = t_upd[0];
                        continue;
                }
                ip_arr[ip_arr_i] = (IP_Frame*)calloc(1, sizeof(IP_Frame));
                ip_arr[ip_arr_i]->timestamp = convert_date_to_epoch(line);
                process_ip_frame(line, ip_arr[ip_arr_i], Enumerator_Addr, Enumerator_Proto);
                free(line);
                line = t_upd[0];
                ip_arr_i++;
                l_count++;
        }
        *filtered = --ip_arr_i;
        free(t_upd);
        return ip_arr;
}

void process_ip_short_input_live(unsigned char live_mode, char *line_buffer, char *out_filename_ptr, unsigned short argument_flags, char *args, Enum_Type *Enumerator_Addr, Enum_Type *Enumerator_Proto){
        unsigned int line_count = 0;
        unsigned int filtered = 0;
	int live_descriptor_write = 0;
        IP_Frame **ip_arr_ptr = process_ip_frame_lines(line_buffer, 1, &filtered, Enumerator_Addr, Enumerator_Proto);
        if (filtered != 0xFFFFFFFF) write_out_frames_new((void*)ip_arr_ptr, 1, '\0', line_count, out_filename_ptr, argument_flags, args);
        if ((filtered != 0xFFFFFFFF) && (live_descriptor_write)){
        write_descriptor(Enumerator_Addr, "ip_addresses.csv", out_filename_ptr, argument_flags, args); write_descriptor(Enumerator_Proto, "ip_protocols.csv", out_filename_ptr, argument_flags, args); live_descriptor_write = 0;}
}
