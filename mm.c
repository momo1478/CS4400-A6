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

/* Get Header from payload pointer bp */ 
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))-OVERHEAD)

#define GET_SIZE(p) ((block_header *)(p))->size
#define GET_ALLOC(p) ((block_header *)(p))->allocated

/* Payload of next block head pointer */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp)- OVERHEAD))

#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))
#define PACK(size, alloc) ((size) | (alloc))

void *current_avail = NULL;
int current_avail_size = 0;
void *start = NULL;

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



void extend(size_t new_size)
{
  size_t chunk_size = PAGE_ALIGN(new_size);
  void *bp = mem_map(chunk_size);
  GET_SIZE(HDRP(bp)) = chunk_size;
  GET_ALLOC(HDRP(bp)) = 0;
  GET_SIZE(HDRP(NEXT_BLKP(bp))) = 0;
  GET_ALLOC(HDRP(NEXT_BLKP(bp))) = 1;
}

/* 
 * examine Memory - prints the state of the allocator.
 */
void examineMemory()
{
  void *bp = start + sizeof(block_header);
  
  //while we haven't hit the terminator.
  while(GET_SIZE(HDRP(bp)) != 0)
  {
    printf("[%d,%d]=[%d]--> ", GET_SIZE(HDRP(bp))/sizeof(block_header), GET_ALLOC(HDRP(bp)), GET_SIZE(FTRP(bp))/sizeof(block_header));
    bp = NEXT_BLKP(bp);
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
  start = current_avail;
  
  void *first_bp = current_avail + sizeof(block_header);

  //Setup Coalescing Prologue.
  GET_SIZE(HDRP(first_bp)) = OVERHEAD;
  GET_ALLOC(HDRP(first_bp)) = 1;
  GET_SIZE(FTRP(first_bp)) = OVERHEAD;

  //Setup (big) initial 0 alloc block
  GET_SIZE(HDRP(NEXT_BLKP(first_bp))) = current_avail_size - OVERHEAD - sizeof(block_header);
  GET_ALLOC(HDRP(NEXT_BLKP(first_bp))) = 0;
  GET_SIZE(FTRP(NEXT_BLKP(first_bp))) = current_avail_size - OVERHEAD - sizeof(block_header);

  GET_SIZE(HDRP(current_avail + current_avail_size)) = 0;

  printf("\n");
  examineMemory();

  if(current_avail)
    return 0;
  else
    return -1;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  int newsize = ALIGN(size);
  void *p;
  
  if (current_avail_size < newsize) 
  {
    current_avail_size = PAGE_ALIGN(newsize);
    current_avail = mem_map(current_avail_size);
    if (current_avail == NULL)
      return NULL;
  }

  p = current_avail;
  current_avail += newsize;
  current_avail_size -= newsize;
  
  return p;
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
  return 0;//GET_ALLOC(HDRP(p)) == 1 && GET_SIZE(HDRP(p)) > 2;
}
