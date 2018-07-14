// Generic functions header file 
#ifndef _ANALYSIS_GENERIC_
#include <stdlib.h>

typedef struct SOURCE_ENUM{
        unsigned short *array;
        unsigned short n;
}SOURCE_ENUM;

typedef struct GLOBAL_KNOWLEDGE{
	SOURCE_ENUM	*Global_Sources;
	SOURCE_ENUM	*Global_Destinations;
	SOURCE_ENUM	*Global_Frames;
}GLOBAL_KNOWLEDGE;

GLOBAL_KNOWLEDGE *global_knowledge_init();

#define _ANALYSIS_GENERIC_
#endif
