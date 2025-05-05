
#include <stdint.h>

//! maximum amount of descriptors allowed
#define MAX_DESCRIPTORS					10


typedef struct tag_gdtDescriptor {
	 uint16_t segmentLimit;
	 uint16_t baseLow;
	 
	 uint8_t baseMiddle;
	 uint8_t flags;
	 uint8_t grand;
	 uint8_t baseHigh;

}gdt_descriptor;

int GDTInitialize();