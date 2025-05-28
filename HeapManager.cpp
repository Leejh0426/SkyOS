#include "HeapManager.h"
#include "SkyConsole.h"
#include "kheap.h"

using namespace VirtualMemoryManager;

namespace HeapManager
{
	int m_heapFrameCount = 0;
	void* m_pKernelHeapPhysicalMemory = 0;

	bool InitKernelHeap(int heapFrameCount)
	{
		PageDirectory* curPageDirectory = GetCurPageDirectory();

		//���� �����ּ�
		void* pVirtualHeap = (void*)(KERNEL_VIRTUAL_HEAP_ADDRESS);

		m_heapFrameCount = heapFrameCount;

		//�����Ӽ���ŭ �����޸��Ҵ��û
		m_pKernelHeapPhysicalMemory = PhysicalMemoryManager::AllocBlocks(m_heapFrameCount);

		if (m_pKernelHeapPhysicalMemory == NULL)
		{
#ifdef _HEAP_DEBUG
			SkyConsole::Print("kernel heap allocation fail. frame count :: %d\n", m_heapFrameCount);
#endif
			return false;
		}

#ifdef _HEAP_DEBUG
		SkyConsole::Print("kernel heap allocationj success. frame count : %d\n", m_herapFrameCount);
#endif
		
		//���� ������ �ּ�
		int virtualEndAddress = (uint32_t)pVirtualHeap + m_heapFrameCount * PMM_BLOCK_SIZE;
		//����¡ �ý��ۿ� �� �����ּҿ� �����ּҸ� �����Ѵ�.
		MapHeapToAddressSpace(curPageDirectory);

		//���� �Ҵ�� �����ּ� ������ ����ؼ� �� �ڷᱸ���� �����Ѵ�.
		create_kernel_heap((u32int)pVirtualHeap, (uint32_t)virtualEndAddress, (uint32_t)virtualEndAddress, 0, 0);
		return true;

	}


	bool MapHeapToAddressSpace(PageDirectory* curPageDirectory) 
	{
		//int endAddress = (uint32_t)KERNEL_VIRTUAL_HEAP_ADDRESS + m_heapFrameCount * PMM_BLOCK_SIZE;
		
		for (int i = 0; i < m_heapFrameCount; i++)
		{
			MapPhysicalAddressToVirtualAddress(curPageDirectory
				, (uint32_t)KERNEL_VIRTUAL_HEAP_ADDRESS + i * PAGE_SIZE
				, (uint32_t)m_pKernelHeapPhysicalMemory + i * PAGE_SIZE
				, I86_PTE_PRESENT | I86_PTE_WRITABLE);

		}

		return true;
	}
}