#include "SkyOS.h"
#include "Exception.h"


namespace PhysicalMemoryManager {
	uint32_t m_memorySize = 0;
	uint32_t m_usedBlocks = 0;

	uint32_t m_maxBlocks = 0;

	uint32_t* m_pMemoryMap = 0;
	uint32_t m_memoryMapSize = 0;

	uint32_t g_totalMemorySize = 0;

	uint32_t GetKernelEnd(multiboot_info* bootinfo)
	{
		uint64_t endAddress = 0;
		uint32_t mods_count = bootinfo->mods_count;   /* Get the amount of modules available */
		uint32_t mods_addr = (uint32_t)bootinfo->Modules;     /* And the starting address of the modules */
		for (uint32_t i = 0; i < mods_count; i++) {
			Module* module = (Module*)(mods_addr + (i * sizeof(Module)));     /* Loop through all modules */

			uint32_t moduleStart = PAGE_ALIGN_DOWN((uint32_t)module->ModuleStart);
			uint32_t moduleEnd = PAGE_ALIGN_UP((uint32_t)module->ModuleEnd);

			if (endAddress < moduleEnd)
			{
				endAddress = moduleEnd;
			}

			SkyConsole::Print("%x %x\n", moduleStart, moduleEnd);
		}

		return (uint32_t)endAddress;
	}

	void Initialize(multiboot_info* bootinfo)
	{
		SkyConsole::Print("Physical Memory Manager Init..\n");

		g_totalMemorySize = GetTotalMemory(bootinfo);

		m_usedBlocks = 0;
		m_memorySize = g_totalMemorySize;
		m_maxBlocks = m_memorySize / PMM_BLOCK_SIZE;

		int pageCount = m_maxBlocks / PMM_BLOCKS_PER_BYTE / PAGE_SIZE;
		if (pageCount == 0) pageCount = 1;

		m_pMemoryMap = (uint32_t*)GetKernelEnd(bootinfo);

		SkyConsole::Print("Total Memory (%dMB)\n", g_totalMemorySize / 1048576);
		SkyConsole::Print("BitMap Start Address(0x%x)\n", m_pMemoryMap);
		SkyConsole::Print("BitMap Size(0x%x)\n", pageCount * PAGE_SIZE);

		m_memoryMapSize = m_maxBlocks / PMM_BLOCKS_PER_BYTE;
		m_usedBlocks = GetTotalBlockCount();

		int tempMemoryMapSize = (GetMemoryMapSize() / 4096) * 4096;

		if (GetMemoryMapSize() % 4096 > 0) tempMemoryMapSize += 4096;

		m_memoryMapSize = tempMemoryMapSize;

		unsigned char flag = 0xff;
		memset((char*)m_pMemoryMap, flag, m_memoryMapSize);
		SetAvailableMemory((uint32_t)m_pMemoryMap, m_memorySize);
	}

	uint32_t GetTotalMemory(multiboot_info* bootinfo) {

	}


	void* AllocBlock() {

		if (GetFreeBlockCount() <= 0) return NULL;

		int frame = GetFreeFrame();
		if (frame == -1) return NULL;

		SetBit(frame);

		uint32_t addr = frame * PMM_BLOCK_SIZE;
		m_usedBlocks++;

		return (void*)addr;

	}

	unsigned int GetFreeFrame() {  //메모리맵은 int 배열, int배열 요소중 / 하나의 인트값의 비트를 보면 /  비트 하나 당 하나의 블록(4KB)을 가리킴
		for (uint32_t i = 0; i < GetTotalBlockCount() / 32; i++) {
			if (m_pMemoryMap[i] != 0xffffffff) {
				for (unsigned int j = 0; j < PMM_BITS_PER_INDEX; j++) {
					unsigned int bit = 1 << j;
					if ((m_pMemoryMap[i] & bit) == 0)
						return i * PMM_BITS_PER_INDEX + j;
				}
			}
		}

		return 0xffffffff;
	}

	void SetBit(int bit) {
		m_pMemoryMap[bit / 32] |= (1 << (bit % 32));
	}
}