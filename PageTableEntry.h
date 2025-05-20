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

	void AddAttribute(PTE* entry, uint32_t attr); //�Ӽ��߰�
	void DelAttribute(PTE* entry, uint32_t attr); //�Ӽ� ����
	void SetFrame(PTE* entry, uint32_t addr); //������ ����
	bool IsPresent(PTE entry); //�޸𸮿� ����?
	bool isWritable(PTE entry); //���� ����?
	uint32_t GetFrame(PTE entry); //���� �ּҸ� ��´�.
}