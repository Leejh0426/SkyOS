#include "SkyOS.h"
#include "Exception.h"


namespace PhysicalMemoryManager {
	uint32_t m_memorySize = 0;
	uint32_t m_usedBlocks = 0;

	//�̿��� �� �ִ� �ִ� �� ����
	uint32_t m_maxBlocks = 0;

	uint32_t* m_pMemoryMap = 0;
	uint32_t m_memoryMapSize = 0;

	uint32_t g_totalMemorySize = 0;

	uint32_t GetTotalMemory(multiboot_info* bootinfo)
	{
		uint64_t endAddress = 0;
		uint32_t mmapEntryNum = bootinfo->mmap_length / sizeof(multiboot_memory_map_t);
		multiboot_mmap_entry* mmapAddr = (multiboot_mmap_entry*)bootinfo->mmap_addr;

		for (uint32_t i = 0; i < mmapEntryNum; i++) {
			uint64_t areaStart = mmapAddr[i].addr;
			uint64_t areaEnd = areaStart + mmapAddr[i].len;

			if (mmapAddr[i].type != 1)
			{
				continue;
			}

			if (areaEnd > endAddress) endAddress = areaEnd;
		}
		if (endAddress > 0xFFFFFFFF)
		{
			endAddress = 0xFFFFFFFF;
		}
		return (uint32_t)endAddress;

	}

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

	uint32_t FindFreeMemory(multiboot_info* info, uint32_t start, int count)
	{
		uint64_t location = start;

		while (location < 0xFFFFFFFF)
		{
			bool notWithinModule = true;

			for (int k = 0; k < count; k++)
			{
				uint32_t pos = location + k * PAGE_SIZE;

				for (uint32_t i = 0; i < info->mods_count; i++)
				{
					Module* module = (Module*)(info->Modules + sizeof(module) * i);
					uint32_t moduleStart = PAGE_ALIGN_DOWN((uint32_t)module->ModuleStart);
					uint32_t moduleEnd = PAGE_ALIGN_UP((uint32_t)module->ModuleEnd);

					SkyConsole::Print("0x%x, 0x%x\n", (uint32_t)moduleStart, (uint32_t)moduleEnd);

					if (pos >= moduleStart && pos < moduleEnd)
					{
						notWithinModule = false;
						location = moduleEnd;
						break;
					}
				}
			}

			if (notWithinModule) return location;

			location += PAGE_SIZE;
		}

		HaltSystem("could not find free memory chunk");
		return 0;
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
		
		//��� �޸𸮺����� ����߿� �ִٰ� �����Ѵ�.
		unsigned char flag = 0xff;
		memset((char*)m_pMemoryMap, flag, m_memoryMapSize);
		//�̿� ������ �޸� ����� �����Ѵ�.(�޸𸮸��� �����ϱ����� �����޸� ������ �����ϰ� ������ ��밡������ �ʱ�ȭ)
		SetAvailableMemory((uint32_t)m_pMemoryMap, m_memorySize);
	}

	uint32_t GetMemoryMapSize() { return m_memoryMapSize; }
	uint32_t GetKernelEnd() { return (uint32_t)m_pMemoryMap; }

	
	uint32_t GetTotalBlockCount()
	{
		return m_maxBlocks;
	}


	void LoadPDBR(uint32_t physicalAddr)
	{
#ifdef _MSC_VER
		_asm
		{
			mov	eax, [physicalAddr]
			mov	cr3, eax		// PDBR is cr3 register in i86
		}
#endif
	}

	uint32_t GetPDBR()
	{

#ifdef _MSC_VER
		_asm
		{
			mov	eax, cr3
			ret
		}
#endif
	}

	bool IsPaging()
	{
		uint32_t res = 0;

#ifdef _MSC_VER
		_asm {
			mov	eax, cr0
			mov[res], eax
		}
#endif

		return (res & 0x80000000) ? false : true;
	}

	void EnablePaging(bool state)
	{
#ifdef _MSC_VER
		_asm
		{
			mov	eax, cr0
			cmp[state], 1
			je	enable
			jmp disable
			enable :
			or eax, 0x80000000		//set bit 31
				mov	cr0, eax
				jmp done
				disable :
			and eax, 0x7FFFFFFF		//clear bit 31
				mov	cr0, eax
				done :
		}
#endif
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

	void FreeBlock(void* p)
	{
		uint32_t addr = (uint32_t)p;
		int frame = addr / PMM_BLOCK_SIZE;
		UnsetBit(frame);
		m_usedBlocks--;
	}

	void* AllocBlocks(size_t size)
	{
		if (GetFreeBlockCount() <= size)
		{
			return NULL;
		}
		
		int frame = GetFreeFrames(size);
		if (frame == -1) {
			return NULL;
		}

		for (uint32_t i = 0; i < size; i++)
		{
			SetBit(frame + i);
		}
		uint32_t addr = frame * PMM_BLOCK_SIZE + (uint32_t)m_pMemoryMap;
		m_usedBlocks += size;

		return(void*)addr;
	}

	void FreeBlocks(void* p, size_t size)
	{
		uint32_t addr = (uint32_t)p - (uint32_t)m_pMemoryMap;
		int frame = addr / PMM_BLOCK_SIZE;

		for (uint32_t i = 0; i < size; i++)
		{
			UnsetBit(frame + i);
		}

		m_usedBlocks -= size;
	}

	size_t GetMemorySize() {

		return m_memorySize;
	}

	uint32_t GetUsedBlockCount() {

		return m_usedBlocks;
	}

	uint32_t GetFreeBlockCount() {

		return m_maxBlocks - m_usedBlocks;
	}

	uint32_t	GetFreeMemory()
	{
		return GetFreeBlockCount() * PMM_BLOCK_SIZE;
	}



	uint32_t GetBlockSize()
	{
		return PMM_BLOCK_SIZE;
	}


	//�ش� ��Ʈ�� ��Ʈ�Ǿ��ִ����Ǿ����� �ʴ����� üũ�Ѵ�.
	//��Ʈ�� ��ȿ���� ����� 0�� �����Ѵ�.
	bool TestMemoryMap(int bit)
	{
		return (m_pMemoryMap[bit / 32] & (1 << (bit % 32))) > 0;
	}

	unsigned int GetFreeFrame() {  //�޸𸮸��� int �迭, int�迭 ����� / �ϳ��� ��Ʈ���� ��Ʈ�� ���� /  ��Ʈ �ϳ� �� �ϳ��� ���(4KB)�� ����Ŵ
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

	//���ӵ� �� ������(��)���� ����
	unsigned int GetFreeFrames(size_t size)
	{
		if (size == 0) return 0xffffffff;

		if (size == 1) return GetFreeFrame();

		for (uint32_t i = 0; i < GetTotalBlockCount() / 32; i++)
		{
			if (m_pMemoryMap[i] != 0xffffffff)
			{
				for (unsigned int j = 0; j < 32; j++)
				{
					unsigned int bit = 1 << j;
					if ((m_pMemoryMap[i] & bit) == 0)
					{
						unsigned int startingBit = i * PMM_BITS_PER_INDEX + j;

						//���ӵ� �� �������� ������ ������Ų��.
						
						uint32_t free = 0;
						for (uint32_t count = 0; count < size; count++) 
						{
							//�޸𸮸��� ����� ��Ȳ
							if (startingBit + count >= m_maxBlocks) return 0xffffffff;
							if (TestMemoryMap(startingBit + count) == false) free++;
							else break;

							if (free == size) return startingBit;
						}
					}
				}
			}
		}
		return 0xffffffff;
	}

	void SetBit(int bit) {
		m_pMemoryMap[bit / 32] |= (1 << (bit % 32));
	}

	//8��° �޸𸮺��� ����� �� ������ ǥ���ϱ� ���� 0���� �����Ϸ��� �迭 ù��° ���(4����Ʈ) ����Ʈ�� 8��° ��Ʈ�� �����ؾ��Ѵ�.
	//00000001000000000000000000000000
	//11111110111111111111111111111111
	//AND �����ڿ� ���� 8��° ��Ʈ�� ������ ��Ʈ�� ��� ���� ���� �����Ѵ�
	void UnsetBit(int bit)
	{
		m_pMemoryMap[bit / 32] &= ~(1 << (bit % 32));
	}


	void SetAvailableMemory(uint32_t base, size_t size)
	{
		int usedBlock = GetMemoryMapSize() / PMM_BLOCK_SIZE; // ��Ʈ���� �����ϴµ� �ʿ��� �޸� �� �� 
		int blocks = GetTotalBlockCount();

		for (int i = usedBlock; i < blocks; i++) //��Ʈ���� ���� �޸𸮴� ������̴� �̸� �����ϰ� ������̶�� �ʱ�ȭ
		{
			UnsetBit(i);
			m_usedBlocks--;
		}
	}

	void SetDeAvailableMemory(uint32_t base, size_t size)
	{
		int align = base / PMM_BLOCK_SIZE;
		int blocks = size / PMM_BLOCK_SIZE;

		for (; blocks > 0; blocks--)
		{
			SetBit(align++);
			m_usedBlocks++;
		}
	}

	void Dump()
	{
		SkyConsole::Print("Memory Size : 0x%x\n", m_memorySize);
		SkyConsole::Print("Memory Map Address : 0x%x\n", m_pMemoryMap);
		SkyConsole::Print("Memory Map Size : %d bytes\n", m_memoryMapSize);
		SkyConsole::Print("Max Block Count : %d\n", m_maxBlocks);

		SkyConsole::Print("Used Block Count : %d\n", m_usedBlocks);
	}
}