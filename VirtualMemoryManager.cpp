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

		if (nullptr == dir)return false;

		//페이지 디렉토리를 PDBR 레지스터에 로드
		SetCurPageDirectory(dir);
		SetKernelPageDirectory(dir);
		SetPageDirectory(dir);

		//페이징 기능을 다시 활성화
		PhysicalMemoryManager::EnablePaging(true);
		return true;
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


				//앞에서 생성한 아이덴티티 페이지 테이블을 PDE(페이지 디렉토리 엔트리)에 세트
				PDE* identityEntry = &dir->m_entries[PAGE_DIRECTORY_INDEX((virt - 0x00400000))];
				PageDirectoryEntry::AddAttribute(identityEntry, I86_PDE_PRESENT | I86_PDE_WRITABLE);
				PageDirectoryEntry::SetFrame(identityEntry, (uint32_t)identityPageTable);
			}
		}

			return dir;

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
}