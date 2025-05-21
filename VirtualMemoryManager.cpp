#include "VirtualMemoryManager.h"
#include "PhysicalMemoryManager.h"
#include "string.h"
#include "memory.h"
#include "SkyConsole.h"
#include "SkyConsole.h"
#include "MultiBoot.h"
#include "SkyAPI.h"

PageDirectory* g_pageDirectoryPool[MAX_PAGE_DIRECTORY_COUNT];
bool g_pageDirectoryAvailable[MAX_PAGE_DIRECTORY_COUNT];

namespace VirtualMemoryManager {

	PageDirectory* _kernel_directory = 0;
	PageDirectory* _cur_directory = 0;


	//���� �ּҿ� ���ε� ���� ���� �ּҸ� ����.
	void* VirtualMemoryManager::GetPhysicalAddressFromVirtualAddress(PageDirectory* directory, uint32_t virtualAddress)
	{
		PDE* pagedir = directory->m_entries;
		if (pagedir[virtualAddress >> 22] == 0)
			return NULL;

		return (void*)((uint32_t*)(pagedir[virtualAddress >> 22] & ~0xfff))[virtualAddress << 10 >> 10 >> 12];
	}

	//���������丮 ��Ʈ�� �ε����� 0�� �ƴϸ� �̹� ������ ���̺��� �����Ѵٴ� �ǹ�
	bool CreatePageTable(PageDirectory* dir, uint32_t virt, uint32_t flags)
	{
		PDE* pageDirectory = dir->m_entries;
		if (pageDirectory[virt >> 22] == 0)
		{
			void* pPageTable = PhysicalMemoryManager::AllocBlock();
			if (pPageTable == nullptr) return false;

			memset(pPageTable, 0, sizeof(PageTable));
			pageDirectory[virt >> 22] = ((uint32_t)pPageTable) | flags;
		}
		return true;
	}


	//PDE�� PTE�� �÷��״� ���� ���� ����
	//�����ּҸ� �����ּҿ� ����
	void MapPhysicalAddressToVirtualAddress(PageDirectory* dir, uint32_t virt, uint32_t phys, uint32_t flags)
	{
		kEnterCriticalSection();
		PhysicalMemoryManager::EnablePaging(false);
		PDE* pageDir = dir->m_entries;

		//������ ���丮�� PDE�� ������ ���̺� �����Ӱ��� ���������� �ʴٸ� ���ο� ������ ���̺��� �����Ѵ�.
		if (pageDir[virt >> 22] == 0)
		{
			CreatePageTable(dir, virt, flags);
		}
		uint32_t mask = (uint32_t)(~0xfff);
		uint32_t* pageTable = (uint32_t*)(pageDir[virt >> 22] & mask);

		//���������̺��� PTE�� ���� �� �����ּҿ� �÷��׸� ����
		//�����ּҿ� �����ּ� ����
		pageTable[virt << 10 >> 10 >> 12] = phys | flags;

		PhysicalMemoryManager::EnablePaging(true);
		kLeaveCriticalSection();

	}

	void MapPhysicalAddressToVirtualAddress2(PageDirectory* dir, uint32_t virt, uint32_t phys, uint32_t flags)
	{
		kEnterCriticalSection();
		PhysicalMemoryManager::EnablePaging(false);

		PDE* pageDir = dir->m_entries;

		if (pageDir[virt >> 22] == 0)
		{
			CreatePageTable(dir, virt, flags);
		}

		uint32_t mask = (uint32_t)(~0xfff);
		uint32_t* pageTable = (uint32_t*)(pageDir[virt >> 22] & mask);

		pageTable[virt << 10 >> 10 >> 12] = phys | flags;

		PhysicalMemoryManager::EnablePaging(true);
		kLeaveCriticalSection();
	}

	void FreePageDirectory(PageDirectory* dir)
	{
		PhysicalMemoryManager::EnablePaging(false);
		PDE* pageDir = dir->m_entries;
		for (int i = 0; i < PAGES_PER_DIRECTORY; i++)
		{
			PDE& pde = pageDir[i];

			if (pde != 0)
			{
				void* frame = (void*)(pageDir[i] & 0x7FFFF000);
				PhysicalMemoryManager::FreeBlock(frame);
				pde = 0;
			}
		}

		for (int index = 0; index < MAX_PAGE_DIRECTORY_COUNT; index++)
		{
			if (g_pageDirectoryPool[index] == dir)
			{
				g_pageDirectoryAvailable[index] = true;
				break;
			}
		}
	}

	void UnmapPageTable(PageDirectory* dir, uint32_t virt)
	{
		PDE* pageDir = dir->m_entries;
		if (pageDir[virt >> 22] != 0)
		{
			void* frame = (void*)(pageDir[virt>>22] & 0x7FFFF000);
			PhysicalMemoryManager::FreeBlock(frame);
			pageDir[virt >> 22] = 0;
		}
	}

	void UnmapPhysicalAddress(PageDirectory* dir, uint32_t virt)
	{
		PDE* pagedir = dir->m_entries;
		if (pagedir[virt >> 22] != 0) UnmapPageTable(dir, virt);
	}

	PageDirectory* CreatePageDirectory()
	{
		PageDirectory* dir = NULL;

		//allocate page directory
		dir = (PageDirectory*)PhysicalMemoryManager::AllocBlock();
		if (!dir) return NULL;

		//memset(dir, 0, sizeof(PageDirectory));

		return dir;
	}

	void ClearPageDirectory(PageDirectory* dir)
	{
		if (dir == NULL) return;

		memset(dir, 0, sizeof(PageDirectory));
	}

	PDE* GetPDE(PageDirectory* dir, uint32_t addr)
	{
		if (dir == NULL) return NULL;

		return &dir->m_entries[GetPageTableIndex(addr)];
	}

	uint32_t GetPageTableIndex(uint32_t addr)
	{
		return (addr >= DTABLE_ADDR_SPACE_SIZE) ? 0 : addr / PAGE_SIZE;
	}

	uint32_t GetPageTableEntryIndex(uint32_t addr)
	{
		return (addr >= PTABLE_ADDR_SPACE_SIZE) ? 0 : addr / PAGE_SIZE;
	}

	PTE* GetPTE(PageTable* p, uint32_t addr)
	{
		if (p == NULL) return NULL;

		return &p->m_entries[GetPageTableEntryIndex(addr)];
	}

	void ClearPageTable(PageTable* p)
	{
		if (p != NULL) memset(p, 0, sizeof(PageTable));
	}

	bool AllocPage(PTE* e) 
	{
		void* p = PhysicalMemoryManager::AllocBlock();

		if (p == NULL) return false;

		PageTableEntry::SetFrame(e, (uint32_t)p);
		PageTableEntry::AddAttribute(e, I86_PTE_PRESENT);

		return true;
	}

	void FreePage(PTE* e)
	{
		void* p = (void*)PageTableEntry::GetFrame(*e);
		if (p) PhysicalMemoryManager::FreeBlock(p);

		PageTableEntry::DelAttribute(e, I86_PTE_PRESENT);
	}


	PageDirectory* CreateCommonPageDirectory()
	{
		//������ ���丮 ����
		//�����ּҰ��� 4GB�� ǥ���ϱ� ���ؼ� ���������丮 �ϳ��� ���
		//������ ���丮�� 1024�� ������ ���̺� ����
		//1024*1024(������ ���̺� ��Ʈ�� ����) * 4KB(�������� ũ��) = 4GB

		int index = 0;
		//���������丮Ǯ���� ����Ҽ��ִ� ������ ���丮 �ϳ� ����.
		for (; index < MAX_PAGE_DIRECTORY_COUNT; index++)
		{
			if (g_pageDirectoryAvailable[index] == true) break;
		}
		if (index == MAX_PAGE_DIRECTORY_COUNT) return nullptr;

		PageDirectory* dir = g_pageDirectoryPool[index];

		if (dir == NULL) return nullptr;

		//�� ������ ���丮�� ��������� ǥ���ϰ� �ʱ�ȭ
		g_pageDirectoryAvailable[index] = false;
		memset(dir, 0, sizeof(PageDirectory));

		uint32_t frame = 0x00000000; //�����ּ� ���۾�巹��
		uint32_t virt = 0x00000000; //�����ּ� ���۾�巹��

		//������ ���̺� ����, ������ ���̺� �ϳ��� 4MB�ּҿ��� ǥ��
		//������ ���̺� �ΰ� �����ϸ�, �����ּҿ� �����ּҰ� ���� ���̵���Ƽ ���� ����

		for (int i = 0; i < 2; i++)
		{
			PageTable* identityPageTable = (PageTable*)PhysicalMemoryManager::AllocBlock();
			if (identityPageTable == NULL)
			{
				return nullptr;
			}
			memset(identityPageTable, 0, sizeof(PageTable));

			//�����ּҸ� �����ּҿ� �����ϰ� ����
			for (int j = 0; j < PAGES_PER_TABLE; j++, frame += PAGE_SIZE, virt += PAGE_SIZE)
			{
				PTE page = 0;
				PageTableEntry::AddAttribute(&page, I86_PTE_PRESENT);
				PageTableEntry::SetFrame(&page, frame);
				identityPageTable->m_entries[PAGE_TABLE_INDEX(virt)] = page;
			}

			//�տ��� ������ ���̵�ƼƼ ������ ���̺��� PDE(������ ���丮 ��Ʈ��)�� ��Ʈ
			PDE* identityEntry = &dir->m_entries[PAGE_DIRECTORY_INDEX((virt - 0x00400000))];
			PageDirectoryEntry::AddAttribute(identityEntry, I86_PDE_PRESENT | I86_PDE_WRITABLE);
			PageDirectoryEntry::SetFrame(identityEntry, (uint32_t)identityPageTable);
			
		}

		return dir;

	}

	void SetPageDirectory(PageDirectory* dir)
	{
		_asm
		{
			mov	eax, [dir]
			mov	cr3, eax		// PDBR is cr3 register in i86
		}
	}


	bool Initialize()
	{
		SkyConsole::Print("Virtual Memory Manger Init...\n");
		//������ ���丮 Ǯ ����, ����Ʈ�� 10��  ����
		for (int i = 0; i < MAX_PAGE_DIRECTORY_COUNT; i++)
		{
			g_pageDirectoryPool[i] = (PageDirectory*)PhysicalMemoryManager::AllocBlock();
			g_pageDirectoryAvailable[i] = true;
		}

		//������ ���丮�� ����, ���� �޼ҵ�� Ŀ�� ���� �ּ� ���α��� �۾�
		PageDirectory* dir = CreateCommonPageDirectory();

		if (nullptr == dir)
			return false;

		
		//������ ���丮�� PDBR �������Ϳ� �ε�
		SetCurPageDirectory(dir);
		SetKernelPageDirectory(dir);
	
		SetPageDirectory(dir);



		//����¡ ����� �ٽ� Ȱ��ȭ
		PhysicalMemoryManager::EnablePaging(true);

		for (;;)
	
		return true;
	}

	bool SetKernelPageDirectory(PageDirectory* dir)
	{
		if (dir == NULL) return false;

		_kernel_directory = dir;

		return true;
	}

	PageDirectory* GetkernelPageDirectory()
	{
		return _kernel_directory;
	}

	bool SetCurPageDirectory(PageDirectory* dir)
	{
		if (dir == NULL)
			return false;

		_cur_directory = dir;

		return true;
	}

	PageDirectory* GetCurPageDirectory()
	{
		return _cur_directory;
	}

	void FlushTranslationLockBufferEntry(uint32_t addr)
	{
#ifdef _MSC_VER
		_asm {
			cli
			invlpg	addr
			sti
		}
#endif
	}

	bool CreateVideoDMAVirtualAddress(PageDirectory* pd, uintptr_t virt, uintptr_t phys, uintptr_t end)
	{
		//void* memory = PhysicalMemoryManager::AllocBlocks((end - start)/ PAGE_SIZE);
		for (int i = 0; virt <= end; virt += 0x1000, phys += 0x1000, i++)
		{
			MapPhysicalAddressToVirtualAddress(pd, (uint32_t)virt, (uint32_t)phys, I86_PTE_PRESENT | I86_PTE_WRITABLE);
		}

		return true;
	}

	void Dump()
	{

	}

}