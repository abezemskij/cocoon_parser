// Header file for structures
#include <stdint.h>

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
        char*   bssid;
        unsigned char essid_len;
        char*   essid;
        unsigned char frame_type;
        unsigned short frame_length;
        unsigned short frame_sn;
        unsigned char  frame_fn;
//      unsigned short frame_flags;
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

