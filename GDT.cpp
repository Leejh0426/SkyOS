#include "gdt.h"
#include "string.h"
#include "memory.h"
#include "windef.h"
#include "defines.h"


typedef struct tag_gdtr {

	uint16_t m_limit;
	uint m_base;

}gdtr;


//! global descriptor table is an array of descriptors
static  gdt_descriptor	_gdt[MAX_DESCRIPTORS];

//! gdtr data
static gdtr				_gdtr;

//! install gdtr
static void InstallGDT() {
#ifdef _MSC_VER
	_asm lgdt[_gdtr]
#endif
}


//! Setup a descriptor in the Global Descriptor Table
void gdt_set_descriptor(uint32_t i, uint64_t base, uint64_t limit, uint8_t access, uint8_t grand)
{
	if (i > MAX_DESCRIPTORS)
		return;

	//! null out the descriptor
	memset((void*)&_gdt[i], 0, sizeof(gdt_descriptor));

	//! set limit and base addresses
	_gdt[i].baseLow = uint16_t(base & 0xffff);
	_gdt[i].baseMiddle = uint8_t((base >> 16) & 0xff);
	_gdt[i].baseHigh = uint8_t((base >> 24) & 0xff);
	_gdt[i].segmentLimit = uint16_t(limit & 0xffff);

	//! set flags and grandularity bytes
	_gdt[i].flags = access;
	_gdt[i].grand = uint8_t((limit >> 16) & 0x0f);
	_gdt[i].grand |= grand & 0xf0;
}


int GDTInitialize() {
	_gdtr.m_limit = (sizeof(struct tag_gdtDescriptor) * MAX_DESCRIPTORS) - 1;
	_gdtr.m_base = (uint32_t)&_gdt[0];

	gdt_set_descriptor(0, 0, 0, 0, 0);

	//커널 디스크립터의 설정
	gdt_set_descriptor(1, 0, 0xffffffff,
		I86_GDT_DESC_READWRITE | I86_GDT_DESC_EXEC_CODE | I86_GDT_DESC_CODEDATA |
		I86_GDT_DESC_MEMORY, I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT |
		I86_GDT_GRAND_LIMITHI_MASK);

	//커널 데이터 디스크립터의 설정
	gdt_set_descriptor(2, 0, 0xffffffff,
		I86_GDT_DESC_READWRITE | I86_GDT_DESC_CODEDATA | I86_GDT_DESC_MEMORY,
		I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT | I86_GDT_GRAND_LIMITHI_MASK);

	//유저모드 디스크립터의 설정
	gdt_set_descriptor(3, 0, 0xffffffff,
		I86_GDT_DESC_READWRITE | I86_GDT_DESC_EXEC_CODE | I86_GDT_DESC_CODEDATA |
		I86_GDT_DESC_MEMORY | I86_GDT_DESC_DPL, I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT |
		I86_GDT_GRAND_LIMITHI_MASK);

	//유저모드 데이터 디스크립터의 설정
	gdt_set_descriptor(4, 0, 0xffffffff, I86_GDT_DESC_READWRITE | I86_GDT_DESC_CODEDATA | I86_GDT_DESC_MEMORY | I86_GDT_DESC_DPL,
		I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT | I86_GDT_GRAND_LIMITHI_MASK);

	//GDTR 레지스터에 GDT 로드
	InstallGDT();

	return 0;
}