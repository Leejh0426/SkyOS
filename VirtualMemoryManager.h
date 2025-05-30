#pragma once
#include "windef.h"
#include "stdint.h"
#include "Hal.h"
#include "PageDirectoryEntry.h"
#include "PageTableEntry.h"

#define USER_VIRTUAL_STACK_ADDRESS 0x00F00000
#define KERNEL_VIRTUAL_HEAP_ADDRESS 0x10000000

using namespace PageTableEntry;
using namespace PageDirectoryEntry;

#define PAGES_PER_TABLE 1024
#define PAGES_PER_DIRECTORY 1024
#define PAGE_TABLE_SIZE 4096

//페이지 테이블 하나당 주소공간 : 4MB
#define PTABLE_ADDR_SPACE_SIZE 0x400000 
//페이지 디렉토리 하나가 표현할 수 있는 주소공간 4GB
#define DTABLE_ADDR_SPACE_SIZE 0x100000000

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3ff) //0x3ff = 00011 1111 1111 10개비트마스킹
#define PAGE_TABLE_INDEX(x) (((x)>>12) & 0x3ff)
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*x & ~0xfff)


#define MAX_PAGE_DIRECTORY_COUNT 40



typedef struct tag_PageTable
{
	PTE m_entries[PAGES_PER_TABLE];
}PageTable;

typedef struct tag_PageDirectory
{
	PDE m_entries[PAGES_PER_DIRECTORY];
}PageDirectory;

typedef struct tag_TaskSwitch
{
	int entryPoint;
	unsigned int procStack;
	LPVOID param;
}TaskSwitch;



namespace VirtualMemoryManager {

	//가상 메모리 초기화
	bool Initialize();

	//페이지(프레임) 할당
	bool AllocPage(PTE* e);
	//페이지(프레임) 회수
	void FreePage(PTE* e);

	//페이지 디렉토리를 생성하고, 커널과의 매핑 작업을 추가로 수행
	PageDirectory* CreateCommonPageDirectory();
	void SetPageDirectory(PageDirectory* dir);

	//페이지 디렉토리를 PDTR레지스터에 세트한다.
	bool SetCurPageDirectory(PageDirectory* dir);

	bool SetKernelPageDirectory(PageDirectory* dir);

	//현재 페이지 디렉토리를 가져온다.
	PageDirectory* GetCurPageDirectory();

	PageDirectory* GetKernelPageDirectory();

	//캐쉬된 TLS 락 버퍼를 비운다.
	void FlushTranslationLockBufferEntry(uint32_t addr);

	//페이지테이블을 초기화
	void ClearPageTable(PageTable* p);
	//페이지 테이블 엔트리(PTE)를 가져온다.
	PTE* GetPTE(PageTable* p, uint32_t addr);
	//주소로부터 PTE를 얻어온다.
	uint32_t GetPageTableEntryIndex(uint32_t addr);
	//주소로부터 페이지 테이블을 얻어온다.
	uint32_t GetPageTableIndex(uint32_t addr);
	//페이지 디렉토리를초기화한다.
	void ClearPageDirectory(PageDirectory* dir);
	//주소로부터 페이지 디렉토리 엔트리를 얻어온다.
	PDE* GetPDE(PageDirectory* p, uint32_t addr);

	//페이지 테이블 생성, 페이지 테이블 크기는 4K
	bool CreatePageTable(PageDirectory* dir, uint32_t virt, uint32_t flags);

	//가상주소를 물리주소에 매핑, 이 과정에서 페이지 테이블 엔트리에 정보가 기록된다.
	void MapPhysicalAddressToVirtualAddress(PageDirectory* dir, uint32_t virt, uint32_t phys, uint32_t flags);
	void MapPhysicalAddressToVirtualAddress2(PageDirectory* dir, uint32_t virt, uint32_t phys, uint32_t flags);

	//가상주소로부터 실제 물리주소를 얻어낸다.
	void* GetPhysicalAddressFromVirtualAddress(PageDirectory* directory, uint32_t virtualAddress);
	//페이지 디렉토리에 매핑된 페이지 디렉토리를 해체한다.
	void UnmapPageTable(PageDirectory* dir, uint32_t virt);
	void UnmapPhysicalAddress(PageDirectory* dir, uint32_t virt);
	void FreePageDirectory(PageDirectory* dir);

	//페이지 디렉토리를 생성한다. 즉 가상주소공간을 생성
	PageDirectory* CreatePageDirectory();

	bool CreateVideoDMAVirtualAddress(PageDirectory* pd, uintptr_t virt, uintptr_t phys, uintptr_t end);

	//Debug
	void Dump();


}