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

#define NEXT_PAGE(p) (((page *)p)->next)
#define PAGE_SIZE(p) (((page *)p)->size)

extern void kill(int,int);

void *current_avail = NULL;
int current_avail_size = 0;

void *first_bp = NULL;
void *first_page = NULL;

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

/* 
 * examine Memory - prints the state of the allocator.
 */
void examineMemory()
{
  void *bp = first_bp;
  void *pg = first_page;

    int pageCount = 0;
    // while(pg != NULL)
    // {
    //   printf("page[%d] @ %p\n",pageCount, pg);
    //   pageCount++;
    //   pg = NEXT_PAGE(pg);
    // }
    while(pg != NULL)
    {
      bp = pg + sizeof(page) + sizeof(block_header);
      printf("[pg : %d|size : %ld]\n", pageCount, PAGE_SIZE(pg));
        //while we haven't hit the terminator in that page
        while(GET_SIZE(HDRP(bp)) != 0)
        {
          printf("[%ld,%d]=[%ld]--> ", GET_SIZE(HDRP(bp))/sizeof(block_header), GET_ALLOC(HDRP(bp)), GET_SIZE(FTRP(bp))/sizeof(block_header));
          bp = NEXT_BLKP(bp);
        }
        printf("[X]\n");

      pg = NEXT_PAGE(pg);
      pageCount++;
    }
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  current_avail_size = PAGE_ALIGN(mem_pagesize());
  current_avail = mem_map(current_avail_size);
  
  //Setup first page
  first_page = current_avail;
  PAGE_SIZE(first_page) = current_avail_size;
  NEXT_PAGE(first_page) = NULL;
  
  //first payload pointer
  first_bp = current_avail + sizeof(page) + sizeof(block_header);

  //Setup Coalescing Prologue.
  GET_SIZE(HDRP(first_bp)) = OVERHEAD;
  GET_ALLOC(HDRP(first_bp)) = 1;
  GET_SIZE(FTRP(first_bp)) = OVERHEAD;

  //Setup (big) initial 0 alloc block
  GET_SIZE(HDRP(NEXT_BLKP(first_bp))) = current_avail_size - OVERHEAD - sizeof(block_header) - sizeof(page);
  GET_ALLOC(HDRP(NEXT_BLKP(first_bp))) = 0;
  GET_SIZE(FTRP(NEXT_BLKP(first_bp))) = current_avail_size - OVERHEAD - sizeof(block_header) - sizeof(page);

  //Setup Terminator Block
  GET_SIZE(HDRP(current_avail + current_avail_size)) = 0;

  mm_malloc(48);
  mm_malloc(90);
  mm_malloc(16);
  mm_malloc(16);
  mm_malloc(100);
  mm_malloc(2048);
  mm_malloc(2048);
  mm_malloc(2048);
  mm_malloc(2048);
  mm_malloc(2048);
  mm_malloc(2048);
  mm_malloc(2048);
  mm_malloc(2048);

  printf("\n");
  printf("\n");
  examineMemory();

  //kill(getpid(), 2);
  if(current_avail)
    return 0;
  else
    return -1;
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

void* extend(size_t new_size)
{
  size_t chunk_size = PAGE_ALIGN(new_size);
  void *bp = mem_map(chunk_size);
  
  void *pg = first_page;
  while(NEXT_PAGE(pg) != NULL)
  {
    pg = NEXT_PAGE(pg);
  }

  NEXT_PAGE(pg) = bp;
  PAGE_SIZE(pg) = chunk_size;
  NEXT_PAGE(NEXT_PAGE(pg)) = NULL;

  bp = NEXT_PAGE(pg) + sizeof(page) + sizeof(block_header);

  //setup prologue and epilogue for new chunk
  GET_SIZE(HDRP(bp)) = OVERHEAD;
  GET_ALLOC(HDRP(bp)) = 1;
  GET_SIZE(FTRP(bp)) = OVERHEAD;

  //setup unallocated block for new chunk.
  GET_SIZE(HDRP(NEXT_BLKP(bp))) = PAGE_SIZE(pg) - OVERHEAD - sizeof(block_header) - sizeof(page);
  GET_ALLOC(HDRP(NEXT_BLKP(bp))) = 0;
  GET_SIZE(FTRP(NEXT_BLKP(bp))) = PAGE_SIZE(pg) - OVERHEAD - sizeof(block_header) - sizeof(page);

  //setup terminator block for new chunk
  GET_SIZE(HDRP(NEXT_BLKP(NEXT_BLKP(bp)))) = 0;
  GET_ALLOC(HDRP(NEXT_BLKP(NEXT_BLKP(bp)))) = 1;

  return bp;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
 int new_size = ALIGN(size + OVERHEAD);
 
 void *pg = first_page;
 void *bp = first_bp;

 while(pg != NULL)
 {
   bp = pg + sizeof(page) + sizeof(block_header);
   while (GET_SIZE(HDRP(bp)) != 0) 
   {
     //printf("new_size : %d, current block size : %ld\n", new_size, GET_SIZE(HDRP(bp)));
     if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp)) >= new_size) ) 
     {
       set_allocated(bp, new_size);
       return bp;
     }
     bp = NEXT_BLKP(bp);
   }
   printf("looking in next page\n");
   pg = NEXT_PAGE(pg);
 }

 printf("extending!\n");
 bp = extend(new_size);

 printf("first_page @ %p\n", first_page);
 printf("bp @ %p\n", bp);
 set_allocated(bp, new_size);
 return bp;
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  GET_ALLOC(HDRP(ptr)) = 0;
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
