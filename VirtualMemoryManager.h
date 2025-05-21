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

//������ ���̺� �ϳ��� �ּҰ��� : 4MB
#define PTABLE_ADDR_SPACE_SIZE 0x400000 
//������ ���丮 �ϳ��� ǥ���� �� �ִ� �ּҰ��� 4GB
#define DTABLE_ADDR_SPACE_SIZE 0x100000000

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3ff) //0x3ff = 00011 1111 1111 10����Ʈ����ŷ
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

	//���� �޸� �ʱ�ȭ
	bool Initialize();

	//������(������) �Ҵ�
	bool AllocPage(PTE* e);
	//������(������) ȸ��
	void FreePage(PTE* e);

	//������ ���丮�� �����ϰ�, Ŀ�ΰ��� ���� �۾��� �߰��� ����
	PageDirectory* CreateCommonPageDirectory();
	void SetPageDirectory(PageDirectory* dir);

	//������ ���丮�� PDTR�������Ϳ� ��Ʈ�Ѵ�.
	bool SetCurPageDirectory(PageDirectory* dir);

	bool SetKernelPageDirectory(PageDirectory* dir);

	//���� ������ ���丮�� �����´�.
	PageDirectory* GetCurPageDirectory();

	PageDirectory* GetKernelPageDirectory();

	//ĳ���� TLS �� ���۸� ����.
	void FlushTranslationLockBufferEntry(uint32_t addr);

	//���������̺��� �ʱ�ȭ
	void ClearPageTable(PageTable* p);
	//������ ���̺� ��Ʈ��(PTE)�� �����´�.
	PTE* GetPTE(PageTable* p, uint32_t addr);
	//�ּҷκ��� PTE�� ���´�.
	uint32_t GetPageTableEntryIndex(uint32_t addr);
	//�ּҷκ��� ������ ���̺��� ���´�.
	uint32_t GetPageTableIndex(uint32_t addr);
	//������ ���丮���ʱ�ȭ�Ѵ�.
	void ClearPageDirectory(PageDirectory* dir);
	//�ּҷκ��� ������ ���丮 ��Ʈ���� ���´�.
	PDE* GetPDE(PageDirectory* p, uint32_t addr);

	//������ ���̺� ����, ������ ���̺� ũ��� 4K
	bool CreatePageTable(PageDirectory* dir, uint32_t virt, uint32_t flags);

	//�����ּҸ� �����ּҿ� ����, �� �������� ������ ���̺� ��Ʈ���� ������ ��ϵȴ�.
	void MapPhysicalAddressToVirtualAddress(PageDirectory* dir, uint32_t virt, uint32_t phys, uint32_t flags);
	void MapPhysicalAddressToVirtualAddress2(PageDirectory* dir, uint32_t virt, uint32_t phys, uint32_t flags);

	//�����ּҷκ��� ���� �����ּҸ� ����.
	void* GetPhysicalAddressFromVirtualAddress(PageDirectory* directory, uint32_t virtualAddress);
	//������ ���丮�� ���ε� ������ ���丮�� ��ü�Ѵ�.
	void UnmapPageTable(PageDirectory* dir, uint32_t virt);
	void UnmapPhysicalAddress(PageDirectory* dir, uint32_t virt);
	void FreePageDirectory(PageDirectory* dir);

	//������ ���丮�� �����Ѵ�. �� �����ּҰ����� ����
	PageDirectory* CreatePageDirectory();

	bool CreateVideoDMAVirtualAddress(PageDirectory* pd, uintptr_t virt, uintptr_t phys, uintptr_t end);

	//Debug
	void Dump();


}