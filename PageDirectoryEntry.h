#pragma once
#include <stdint.h>

namespace PageDirectoryEntry
{
	typedef uint32_t PDE;

	enum PAGE_PDE_FLAGS {

		I86_PDE_PRESENT = 1,			
		I86_PDE_WRITABLE = 2,			
		I86_PDE_USER = 4,			
		I86_PDE_PWT = 8,			
		I86_PDE_PCD = 0x10,		
		I86_PDE_ACCESSED = 0x20,
		I86_PDE_DIRTY = 0x40,	
		I86_PDE_4MB = 0x80,		
		I86_PDE_CPU_GLOBAL = 0x100,		
		I86_PDE_LV4_GLOBAL = 0x200,		
		I86_PDE_FRAME = 0x7FFFF000 	
	};

	void AddAttribute(PDE* entry, uint32_t attr);
	void DelAttribute(PDE* entry, uint32_t attr);
	void SetFrame(PDE* entry, uint32_t addr);
	bool IsPresent(PDE entry);
	bool IsWritable(PDE entry);
	uint32_t GetFrame(PDE entry);
	bool IsUser(PDE entry);
	bool Is4mb(PDE entry);
};