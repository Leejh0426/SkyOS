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
		//������ ���丮 Ǯ ����, ����Ʈ�� 10��  ����
		for (int i = 0; i < MAX_PAGE_DIRECTORY_COUNT; i++)
		{
			g_pageDirectoryPool[i] = (PageDirectory*)PhysicalMemoryManager::AllocBlock();
			g_pageDirectoryAvailable[i] = true;
		}

		//������ ���丮�� ����, ���� �޼ҵ�� Ŀ�� ���� �ּ� ���α��� �۾�
		PageDirectory* dir = CreateCommonPageDirectory();

		if (nullptr == dir)return false;

		//������ ���丮�� PDBR �������Ϳ� �ε�
		SetCurPageDirectory(dir);
		SetKernelPageDirectory(dir);
		SetPageDirectory(dir);

		//����¡ ����� �ٽ� Ȱ��ȭ
		PhysicalMemoryManager::EnablePaging(true);
		return true;
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


				//�տ��� ������ ���̵�ƼƼ ������ ���̺��� PDE(������ ���丮 ��Ʈ��)�� ��Ʈ
				PDE* identityEntry = &dir->m_entries[PAGE_DIRECTORY_INDEX((virt - 0x00400000))];
				PageDirectoryEntry::AddAttribute(identityEntry, I86_PDE_PRESENT | I86_PDE_WRITABLE);
				PageDirectoryEntry::SetFrame(identityEntry, (uint32_t)identityPageTable);
			}
		}

			return dir;

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
}