#include "kheap.h"


// end is defined in the linker script.
//extern u32int end;
//u32int placement_address = (u32int)&end;
heap_t kheap;
DWORD g_usedHeapSize = 0;


u32int kmalloc_int(u32int sz,int align, u32int* phys)
{
	void* addr = memory_alloc(sz, (u8int)align, &kheap);
	return (u32int)addr;
}

void kfree(void* p)
{
	EnterCriticalSection();
	free(p, &kheap);
	LeaveCriticalSection();
}


static void expand(u32int new_size, heap_t* heap)
{
	// Sanity check.
	SKY_ASSERT(new_size > heap->end_address - heap->start_address, "new_size > heap->end_address - heap->start_address");

	// Get the nearest following page boundary.
	if ((new_size & 0xFFFFF000) != 0)
	{
		new_size &= 0xFFFFF000;
		new_size += 0x1000;
	}

	// Make sure we are not overreaching ourselves.
	SKY_ASSERT(heap->start_address + new_size <= heap->max_address, "heap->start_address+new_size <= heap->max_address");

	// This should always be on a page boundary.
	u32int old_size = heap->end_address - heap->start_address;

	u32int i = old_size;
	while (i < new_size)
	{
		// alloc_frame( get_page(heap->start_address+i, 1, kernel_directory),
 //                     (heap->supervisor)?1:0, (heap->readonly)?0:1);
		i += 0x1000 /* page size */;
	}
	heap->end_address = heap->start_address + new_size;
}

static s32int find_smallest_hole(u32int size, u8int page_align, heap_t* heap)
{
	// Find the smallest hole that will fit.
	u32int iterator = 0;
	while (iterator < heap->index.size)
	{
		header_t* header = (header_t*)lookup_ordered_array(iterator, &heap->index);
		// If the user has requested the memory be page-aligned
		if (page_align > 0)
		{
			// Page-align the starting point of this header.
			u32int location = (u32int)header;
			s32int offset = 0;
			if (((location + sizeof(header_t)) & 0xFFFFF000) != 0)
				offset = 0x1000 /* page size */ - (location + sizeof(header_t)) % 0x1000;
			s32int hole_size = (s32int)header->size - offset;
			// Can we fit now?
			if (hole_size >= (s32int)size)
				break;
		}
		else if (header->size >= size)
			break;
		iterator++;
	}
	// Why did the loop exit?
	if (iterator == heap->index.size)
		return -1; // We got to the end and didn't find anything.
	else
		return iterator;
}

void* memory_alloc(u32int size, u8int page_align, heap_t* heap)
{
	// 사이즈 = 내용물 + 헤더 + 풋터
	u32int new_size = size + sizeof(header_t) + sizeof(footer_t);

	s32int iterator = find_smallest_hole(new_size, page_align, heap);

	if (iterator == -1) //적절한 hole발견 못햇다면 공간 늘린다.
	{
		u32int old_length = heap->end_address - heap->start_address;
		u32int old_end_address = heap->end_address;

		expand(old_length + new_size, heap);
		u32int new_length = heap->end_address - heap->start_address;

		//endmost header를 찾는다.
		iterator = 0;
		u32int idx = 0xFFFFFFFF; u32int value = 0x0;
		while (iterator < (s32int)heap->index.size)
		{
			u32int tmp = (u32int)lookup_ordered_array(iterator, &heap->index);
			if (tmp > value)
			{
				value = tmp;
				idx = iterator;
			}
			iterator++;
		}

		// header를 못찾았다면 추가해야한다.
		if(idx==-1)
		{
			header_t* header = (header_t*)old_end_address;
			header->magic = HEAP_MAGIC:
			header->size = new_length - old_length;
			header->is_hole = 1;
			footer_t* footer = (footer_t*)(old_end_address + header->size - sizeof(footer_t));
			footer->magic = HEAP_MAGIC;
			footer->header = header;
			insert_ordered_array((void*)header, &heap->index);
		}
		else
		{
			//마지막 헤더는 조절이 필요하다
			header_t* header = (header_t*)lookup_ordered_array(idx, &heap->index);
			header->size += new_length - old_length;
			//footer 재설정
			footer_t* footer = (footer_t*)((u32int)header + header->size - sizeof(footer_t));
			footer->header = header;
			footer->magic = HEAP_MAGIC;

		}

		//공간을 다시 확보했으므로 함수 재호출.
		return memory_alloc(size, page_align, heap);
	} 

	header_t* orig_hole_header = (header_t*)lookup_ordered_array(iterator, &heap->index);
	u32int orig_hole_pos = (u32int)orig_hole_header;
	u32int orig_hole_size = orig_hole_header->size;

	//우리가 찾은 홀을 두개로 쪼갤 필요가 있다면 여기서 쪼갠다?
	if (orig_hole_size - new_size < sizeof(header_t) + sizeof(footer_t))
	{
		size += orig_hole_size - new_size;
		new_size = orig_hole_size;
	}

	// 데이터가 페이지 얼라인이 필요하면 지금 수행, 홀을 블락 앞으로 위치
	if (page_align && orig_hole_pos & 0xFFFFF000)
	{
		u32int new_location = orig_hole_pos + 0x1000 - (orig_hole_pos & 0xFFF) - sizeof(header_t);
		header_t* hole_header = (header_t*)orig_hole_pos;
		hole_header->size = 0x1000 - (orig_hole_pos & 0xFFF) - sizeof(header_t);
		hole_header->magic = HEAP_MAGIC;
		hole_header->is_hole = 1;
		footer_t* hole_footer = (footer_t*)((u32int)new_location - sizeof(footer_t));
		hole_footer->magic = HEAP_MAGIC;
		hole_footer->header = hole_header;
		orig_hole_pos = new_location;
		orig_hole_size = orig_hole_size - hole_header->size;
	}
	else
	{
		//이 홀이 더이상 필요없으면, 인덱스에서 이 홀을 삭제한다.
		remove_ordered_array(iterator, &heap->index);
	}

	//original header에 overwirte

	header_t* block_header = (header_t*)orig_hole_pos;
	header_t* block_header = (header_t*)orig_hole_pos;
	block_header->magic = HEAP_MAGIC;
	block_header->is_hole = 0;
	block_header->size = new_size;
	// ...And the footer
	footer_t* block_footer = (footer_t*)(orig_hole_pos + sizeof(header_t) + size);
	block_footer->magic = HEAP_MAGIC;
	block_footer->header = block_header;


	// We may need to write a new hole after the allocated block.
	// We do this only if the new hole would have positive size...
	if (orig_hole_size - new_size > 0)
	{
		header_t* hole_header = (header_t*)(orig_hole_pos + sizeof(header_t) + size + sizeof(footer_t));
		hole_header->magic = HEAP_MAGIC;
		hole_header->is_hole = 1;
		hole_header->size = orig_hole_size - new_size;
		footer_t* hole_footer = (footer_t*)((u32int)hole_header + orig_hole_size - new_size - sizeof(footer_t));
		if ((u32int)hole_footer < heap->end_address)
		{
			hole_footer->magic = HEAP_MAGIC;
			hole_footer->header = hole_header;
		}
		// Put the new hole in the index;
		insert_ordered_array((void*)hole_header, &heap->index);
	}

	// ...And we're done!
	return (void*)((u32int)block_header + sizeof(header_t));
}