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


	//가상 주소와 매핑된 실제 물리 주소를 얻어낸다.
	void* VirtualMemoryManager::GetPhysicalAddressFromVirtualAddress(PageDirectory* directory, uint32_t virtualAddress)
	{
		PDE* pagedir = directory->m_entries;
		if (pagedir[virtualAddress >> 22] == 0)
			return NULL;

		return (void*)((uint32_t*)(pagedir[virtualAddress >> 22] & ~0xfff))[virtualAddress << 10 >> 10 >> 12];
	}

	//페이지디렉토리 엔트리 인덱스가 0이 아니면 이미 페이지 테이블이 존재한다는 의미
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


	//PDE나 PTE의 플래그는 같은 값을 공유
	//가상주소를 물리주소에 매핑
	void MapPhysicalAddressToVirtualAddress(PageDirectory* dir, uint32_t virt, uint32_t phys, uint32_t flags)
	{
		kEnterCriticalSection();
		PhysicalMemoryManager::EnablePaging(false);
		PDE* pageDir = dir->m_entries;

		//페이지 디렉토리의 PDE에 페이제 테이블 프레임값이 설정돼있지 않다면 새로운 페이지 테이블을 생성한다.
		if (pageDir[virt >> 22] == 0)
		{
			CreatePageTable(dir, virt, flags);
		}
		uint32_t mask = (uint32_t)(~0xfff);
		uint32_t* pageTable = (uint32_t*)(pageDir[virt >> 22] & mask);

		//페이지테이블에서 PTE를 구한 뒤 물리주소와 플래그를 설정
		//가상주소와 물리주소 매핑
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
		//페이지 디렉토리 생성
		//가상주소공간 4GB를 표현하기 위해서 페이지디렉토리 하나면 충분
		//페이지 디렉토리는 1024개 페이지 테이블 가짐
		//1024*1024(페이지 테이블 엔트리 개수) * 4KB(프레임의 크기) = 4GB

		int index = 0;
		//페이지디렉토리풀에서 사용할수있는 페이지 디렉토리 하나 얻어낸다.
		for (; index < MAX_PAGE_DIRECTORY_COUNT; index++)
		{
			if (g_pageDirectoryAvailable[index] == true) break;
		}
		if (index == MAX_PAGE_DIRECTORY_COUNT) return nullptr;

		PageDirectory* dir = g_pageDirectoryPool[index];

		if (dir == NULL) return nullptr;

		//얻어낸 페이지 디렉토리는 사용중임을 표시하고 초기화
		g_pageDirectoryAvailable[index] = false;
		memset(dir, 0, sizeof(PageDirectory));

		uint32_t frame = 0x00000000; //물리주소 시작어드레스
		uint32_t virt = 0x00000000; //가상주소 시작어드레스

		//페이지 테이블 생성, 페이지 테이블 하나는 4MB주소영역 표현
		//페이지 테이블 두개 생성하며, 가상주소와 물리주소가 같은 아이덴터티 매핑 수행

		for (int i = 0; i < 2; i++)
		{
			PageTable* identityPageTable = (PageTable*)PhysicalMemoryManager::AllocBlock();
			if (identityPageTable == NULL)
			{
				return nullptr;
			}
			memset(identityPageTable, 0, sizeof(PageTable));

			//물리주소를 가상주소와 동일하게 매핑
			for (int j = 0; j < PAGES_PER_TABLE; j++, frame += PAGE_SIZE, virt += PAGE_SIZE)
			{
				PTE page = 0;
				PageTableEntry::AddAttribute(&page, I86_PTE_PRESENT);
				PageTableEntry::SetFrame(&page, frame);
				identityPageTable->m_entries[PAGE_TABLE_INDEX(virt)] = page;
			}

			//앞에서 생성한 아이덴티티 페이지 테이블을 PDE(페이지 디렉토리 엔트리)에 세트
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
		//페이지 디렉토리 풀 생성, 디폴트로 10개  선언
		for (int i = 0; i < MAX_PAGE_DIRECTORY_COUNT; i++)
		{
			g_pageDirectoryPool[i] = (PageDirectory*)PhysicalMemoryManager::AllocBlock();
			g_pageDirectoryAvailable[i] = true;
		}

		//페이지 디렉토리를 생성, 다음 메소드는 커널 영역 주소 매핑까지 작업
		PageDirectory* dir = CreateCommonPageDirectory();

		if (nullptr == dir)
			return false;

		
		//페이지 디렉토리를 PDBR 레지스터에 로드
		SetCurPageDirectory(dir);
		SetKernelPageDirectory(dir);
	
		SetPageDirectory(dir);



		//페이징 기능을 다시 활성화
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