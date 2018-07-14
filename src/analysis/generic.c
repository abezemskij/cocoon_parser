// Generic function implementation

#include "generic.h"

GLOBAL_KNOWLEDGE *global_knowledge_init(){
	GLOBAL_KNOWLEDGE *_glob = (GLOBAL_KNOWLEDGE*)calloc(1, sizeof(GLOBAL_KNOWLEDGE));	// create structure with all 0's
	_glob->Global_Sources = (SOURCE_ENUM*)calloc(1, sizeof(SOURCE_ENUM));
	_glob->Global_Destinations = (SOURCE_ENUM*)calloc(1, sizeof(SOURCE_ENUM));
	_glob->Global_Frames = (SOURCE_ENUM*)calloc(1, sizeof(SOURCE_ENUM));
	return _glob;
}
