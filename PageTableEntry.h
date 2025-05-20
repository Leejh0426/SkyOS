#pragma once
#include <stdint.h>

namespace PageTableEntry
{
	typedef uint32_t PTE;

	enum PAGE_PTE_FLAGS {
		I86_PTE_PRESENT = 1,
		//00000000000000000000000000000001
		I86_PTE_WRITABLE = 2,
		I86_PTE_USER = 4,
		I86_PTE_WRITEHOUGH = 8,
		I86_PTE_CACHABLE = 0x10, 
		I86_PTE_ACCESSED = 0x20, 
		I86_PTE_DIRTY = 0x40, 
		I86_PTE_PAT = 0X80,
		I86_PTE_CPU_GLOBAL = 0X100,
		I86_PTE_LV4_GLOBAL = 0X200,
		I86_PTE_FRAME = 0X7FFFF000
	};

	void AddAttribute(PTE* entry, uint32_t attr); //속성추가
	void DelAttribute(PTE* entry, uint32_t attr); //속성 삭제
	void SetFrame(PTE* entry, uint32_t addr); //프레임 설정
	bool IsPresent(PTE entry); //메모리에 존재?
	bool isWritable(PTE entry); //쓰기 가능?
	uint32_t GetFrame(PTE entry); //물리 주소를 얻는다.
}