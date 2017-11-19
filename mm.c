/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * The heap check and free check always succeeds, because the
 * allocator doesn't depend on any of the old data.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* always use 16-byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

#define OVERHEAD (sizeof(block_header)+sizeof(block_footer))
#define NEW_UBLOCK_SIZE(pgsize) (pgsize - OVERHEAD - sizeof(block_header) - sizeof(page))

/* Get Header from payload pointer bp */ 
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))-OVERHEAD)

#define GET_SIZE(p) ((block_header *)(p))->size
#define GET_ALLOC(p) ((block_header *)(p))->allocated

/* Payload of next block head pointer */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp)- OVERHEAD))

#define NEXT_PAGE(p) (((page *)p)->next)
#define PAGE_SIZE(p) (((page *)p)->size)

typedef struct
{
  void *next;
  size_t size;
} page;

typedef struct
{
  size_t size;    //In Bytes
  char allocated; 
} block_header;
  
typedef struct
{
  size_t size;    //In Bytes
  int filler;
} block_footer;

void *current_avail = NULL; //REMOVE
int current_avail_size = 0; //REMOVE

void* first_page; //First chunk pointer
void* first_pp;   //First payload pointer.


void examineMemory()
{
  void *pp = first_pp;
  
  //while we haven't hit the terminator in that page
  while(GET_SIZE(HDRP(pp)) != 0)
  {
    printf("[%ld,%d]=[%ld]--> ", GET_SIZE(HDRP(pp))/sizeof(block_header), GET_ALLOC(HDRP(pp)), GET_SIZE(FTRP(pp))/sizeof(block_header));
    pp = NEXT_BLKP(pp);
  }
  printf("[X]\n");
}
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  current_avail_size = PAGE_ALIGN(mem_pagesize());
  current_avail = mem_map(current_avail_size);

  //First Page Setup
  first_page = current_avail;
  NEXT_PAGE(first_page) = NULL;
  PAGE_SIZE(first_page) = current_avail_size;

  //First payload pointer
  first_pp = current_avail + sizeof(page) + sizeof(block_header);

  void* pp = first_pp;
  //Setup Coalescing Prologue.
  GET_SIZE(HDRP(pp)) = OVERHEAD;
  GET_ALLOC(HDRP(pp)) = 1;
  GET_SIZE(FTRP(pp)) = OVERHEAD;

  pp = NEXT_BLKP(pp);
  //setup unallocated block for new chunk.
  GET_SIZE(HDRP(pp)) = NEW_UBLOCK_SIZE(PAGE_SIZE(first_page));
  GET_ALLOC(HDRP(pp)) = 0;
  GET_SIZE(FTRP(pp)) = NEW_UBLOCK_SIZE(PAGE_SIZE(first_page));

  pp = NEXT_BLKP(pp);
  //setup terminator block for new chunk
  GET_SIZE(HDRP(pp)) = 0;
  GET_ALLOC(HDRP(pp)) = 1;

  mm_malloc(48);
  mm_malloc(96);
  mm_malloc(256);
  mm_malloc(1024);

  printf("\n");
  printf("\n");
  examineMemory();
  printf("\n");
  return 0;
}

void* extend(size_t new_size) 
{
 size_t chunk_size = PAGE_ALIGN(new_size);
 void *new_page = mem_map(chunk_size);

 PAGE_SIZE(new_page) = chunk_size;
 NEXT_PAGE(new_page) = NULL;

 void *pp = new_page + sizeof(page) + sizeof(block_header);

 //create prologue for new page
 GET_SIZE(HDRP(pp)) = 2;
 GET_ALLOC(HDRP(pp)) = 1;
 GET_SIZE(FTRP(pp)) = 2;

 pp = NEXT_BLKP(pp);

 //create unalocated block
 GET_SIZE(HDRP(pp)) = NEW_UBLOCK_SIZE(PAGE_SIZE(new_page));
 GET_SIZE(FTRP(pp)) = NEW_UBLOCK_SIZE(PAGE_SIZE(new_page));
 GET_ALLOC(HDRP(pp)) = 0;

 pp = NEXT_BLKP(pp);

 //create terminator block
 GET_SIZE(HDRP(pp)) = 0;
 GET_ALLOC(HDRP(pp)) = 1;

 return (new_page + sizeof(page) + OVERHEAD + sizeof(block_header));
}

void set_allocated(void *bp, size_t size) 
{
 size_t extra_size = GET_SIZE(HDRP(bp)) - size;
 if (extra_size > ALIGN(1 + OVERHEAD)) 
 {
   GET_SIZE(HDRP(bp)) = size;
   GET_SIZE(FTRP(bp)) = size;

   GET_SIZE(HDRP(NEXT_BLKP(bp))) = extra_size;
   GET_SIZE(FTRP(NEXT_BLKP(bp))) = extra_size;

   GET_ALLOC(HDRP(NEXT_BLKP(bp))) = 0;
 }
 GET_ALLOC(HDRP(bp)) = 1;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size) 
{
 int new_size = ALIGN(size + OVERHEAD);
 void *bp = first_pp;
 while (GET_SIZE(HDRP(bp)) != 0)
 {
   if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp)) >= new_size))
   {
     set_allocated(bp, new_size);
     return bp;
   }
   bp = NEXT_BLKP(bp);
 }
 bp = extend(new_size);
 set_allocated(bp, new_size);
 return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{

}

/*
 * mm_check - Check whether the heap is ok, so that mm_malloc()
 *            and proper mm_free() calls won't crash.
 */
int mm_check()
{
  return 1;
}

/*
 * mm_check - Check whether freeing the given `p`, which means that
 *            calling mm_free(p) leaves the heap in an ok state.
 */
int mm_can_free(void *p)
{
  return 1;
}
