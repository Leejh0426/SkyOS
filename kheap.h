#pragma once
#include "windef.h"
#include "ordered_array.h"


#define HEAP_INDEX_SIZE   0x20000
#define HEAP_MAGIC        0x123890AB
#define HEAP_MIN_SIZE     0x70000


/**
   Size information for a hole/block
**/
typedef struct
{
    u32int magic;   // Magic number, used for error checking and identification.
    u8int is_hole;   // 1 if this is a hole. 0 if this is a block.
    u32int size;    // size of the block, including the end footer.
} header_t;

typedef struct
{
    u32int magic;     // Magic number, same as in header_t.
    header_t* header; // Pointer to the block header.
} footer_t;

typedef struct
{
    ordered_array_t index;
    u32int start_address; // The start of our allocated space.
    u32int end_address;   // The end of our allocated space. May be expanded up to max_address.
    u32int max_address;   // The maximum address the heap can be expanded to.
    u8int supervisor;     // Should extra pages requested by us be mapped as supervisor-only?
    u8int readonly;       // Should extra pages requested by us be mapped as read-only?
} heap_t;


heap_t* create_kernel_heap(u32int start, u32int end, u32int max, u8int supervisor, u8int readonly);// 커널 힙 생성

heap_t* create_heap(u32int start, u32int end, u32int max, u8int supervisor, u8int readonly); //프로세스 힙 생성

//void* alloc(u32int size, u8int page_align, heap_t* heap); //메모리 할당
void* memory_alloc(u32int size, u8int page_align, heap_t* heap);
void free(void* p, heap_t* heap);

u32int kmalloc(u32int sz); // alloc 함수 호출, 동기화 적용되어 있다.
void kfree(void* p); //kfree함수 호출. 동기화 적용되어 있다
