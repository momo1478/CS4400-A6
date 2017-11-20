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

/* rounds down to the nearest multiple of mem_pagesize() */
#define ADDRESS_PAGE_START(p) ((void *)(((size_t)p) & ~(mem_pagesize()-1)))

#define OVERHEAD (sizeof(block_header)+sizeof(block_footer))
#define BHSIZE (sizeof(block_header))
#define PGSIZE (sizeof(page))
#define MAX_BLOCK_SIZE 1 << 32

#define NEXT_PAGE(pg) (((page *)pg)->next)
#define PREV_PAGE(pg) (((page *)pg)->prev)

/* Get Header from payload pointer bp */ 
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))-OVERHEAD)

#define GET_SIZE(p) ((block_header *)(p))->size
#define GET_ALLOC(p) ((block_header *)(p))->allocated

/* Payload of next block head pointer */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp)- OVERHEAD))



typedef struct
{
  void *next;
  void *prev;
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

page* first_page; //First chunk pointer
void* first_pp;   //First payload pointer.
page* last_page_inserted;


void examineMemory()
{
  void* pp = first_pp;
  void* pg = first_page;

  int pageCount = 0;

  while(pg != NULL)
    {
      pp = (void *)pg + PGSIZE + BHSIZE;
      printf("[pg : %d]\n", pageCount);
      
      while(GET_SIZE(HDRP(pp)) != 0)
        {
          printf("[%ld,%d]=[%ld]--> ", GET_SIZE(HDRP(pp))/sizeof(block_header), GET_ALLOC(HDRP(pp)), GET_SIZE(FTRP(pp))/sizeof(block_header));
          pp = NEXT_BLKP(pp);
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
  size_t firstPageSize = PAGE_ALIGN(mem_pagesize());
  first_page = mem_map(firstPageSize);
  last_page_inserted = first_page;
  
  //First Page Setup
  NEXT_PAGE(first_page) = NULL;
  PREV_PAGE(first_page) = NULL;

  //First payload pointer
  first_pp = (char *)first_page + PGSIZE + BHSIZE;

  void* pp = first_pp;
  //Setup Coalescing Prologue.
  GET_SIZE(HDRP(pp)) = OVERHEAD;
  GET_ALLOC(HDRP(pp)) = 1;
  GET_SIZE(FTRP(pp)) = OVERHEAD;

  pp = NEXT_BLKP(pp);
  //setup unallocated block for new chunk.
  GET_SIZE(HDRP(pp)) = firstPageSize - PGSIZE -  OVERHEAD - BHSIZE;
  GET_ALLOC(HDRP(pp)) = 0;
  GET_SIZE(FTRP(pp)) = firstPageSize - PGSIZE -  OVERHEAD - BHSIZE;

  pp = NEXT_BLKP(pp);
  //setup terminator block for new chunk
  GET_SIZE(HDRP(pp)) = 0;
  GET_ALLOC(HDRP(pp)) = 1;


  // printf("\n");
  // printf("\n");
  // examineMemory();
  // printf("\n");
  return 0;
}

void* extend(size_t new_size) 
{
int clampedSize = new_size > (8 * mem_pagesize()) ? new_size : 8 * mem_pagesize();
 size_t chunk_size = PAGE_ALIGN(clampedSize + 1); //PAGE_ALIGN(clampedSize * 4);
 void *new_page = mem_map(chunk_size);

 //Find pageList end.
 void *pg = last_page_inserted;
 while(NEXT_PAGE(pg) != NULL)
 {
  pg = NEXT_PAGE(pg);
 }

 //Hookup new page into pageList.
 NEXT_PAGE(pg) = new_page;
 NEXT_PAGE(NEXT_PAGE(pg)) = NULL;
 PREV_PAGE(NEXT_PAGE(pg)) = pg;

 last_page_inserted = NEXT_PAGE(pg);

 void *pp = new_page + PGSIZE + BHSIZE;

 //create prologue for new page
 GET_SIZE(HDRP(pp)) = OVERHEAD;
 GET_ALLOC(HDRP(pp)) = 1;
 GET_SIZE(FTRP(pp)) = OVERHEAD;

 pp = NEXT_BLKP(pp); 
 //create unalocated block
 GET_SIZE(HDRP(pp)) = chunk_size - PGSIZE - OVERHEAD - BHSIZE;
 GET_SIZE(FTRP(pp)) = chunk_size - PGSIZE - OVERHEAD - BHSIZE;
 GET_ALLOC(HDRP(pp)) = 0;

 pp = NEXT_BLKP(pp);
 //create terminator block
 GET_SIZE(HDRP(pp)) = 0;
 GET_ALLOC(HDRP(pp)) = 1;

 return (new_page + PGSIZE + OVERHEAD + BHSIZE);
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
  if(size == 0)
    return NULL;
  
 int new_size = ALIGN(size + OVERHEAD);
 void *pg = first_page;
 void *pp;
 while(pg != NULL)
 {
   pp = pg + PGSIZE + OVERHEAD + BHSIZE;
   while (GET_SIZE(HDRP(pp)) != 0)
   {
    if (!GET_ALLOC(HDRP(pp)) && (GET_SIZE(HDRP(pp)) >= new_size))
    {
      set_allocated(pp, new_size);
      return pp;
    }
    pp = NEXT_BLKP(pp);
  }
  pg = NEXT_PAGE(pg);
}
 
 pp = extend(new_size);
 set_allocated(pp, new_size);
 return pp;
}

void *coalesce(void *bp)
{
 size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
 size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
 size_t size = GET_SIZE(HDRP(bp));

 if (prev_alloc && next_alloc)
   { /* Case 1 */
     /* nothing to do */
   }
 else if (prev_alloc && !next_alloc)
   { /* Case 2 */
     size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
     GET_SIZE(HDRP(bp)) = size;
     GET_SIZE(FTRP(bp)) = size;
   }
 else if (!prev_alloc && next_alloc)
   { /* Case 3 */
     size += GET_SIZE(HDRP(PREV_BLKP(bp)));
     GET_SIZE(FTRP(bp)) = size;
     GET_SIZE(HDRP(PREV_BLKP(bp))) = size;
     bp = PREV_BLKP(bp);
   }
 else
   { /* Case 4 */
     size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))));
     GET_SIZE(HDRP(PREV_BLKP(bp))) = size;
     GET_SIZE(FTRP(NEXT_BLKP(bp))) = size;
     bp = PREV_BLKP(bp);
   }

 return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  GET_ALLOC(HDRP(ptr)) = 0;
  coalesce(ptr);
}

int ptr_is_mapped(void *p, size_t len) 
{
    void *s = ADDRESS_PAGE_START(p);
    return mem_is_mapped(s, PAGE_ALIGN((p + len) - s));
}

/*
 * mm_check - Check whether the heap is ok, so that mm_malloc()
 *            and proper mm_free() calls won't crash.
 */
int mm_check()
{
  void* pg = first_page;
  void* pp;

  while(pg != NULL)
  {
    //beginning of page doesn't have atleast 1 page on it.
    if(!ptr_is_mapped(pg,mem_pagesize()))
      return 0;

    pp = (void *)pg + PGSIZE + BHSIZE;

    while(GET_SIZE(HDRP(pp)) != 0)
    {
      // Size is invalid or leads to unmapped address.
      if( !ptr_is_mapped(pp,GET_SIZE(HDRP(pp))) && GET_SIZE(HDRP(pp)) < (size_t)MAX_BLOCK_SIZE)
        return 0;

      //Allocation bit is not 0 or 1
      if(GET_ALLOC(HDRP(pp)) != 0 || GET_ALLOC(HDRP(pp)) != 1)
        return 0;

      //Header size is not the same as footer size.
      if( GET_SIZE(HDRP(pp)) != GET_SIZE(FTRP(pp)) )
        return 0;

      pp = NEXT_BLKP(pp);
    }
    pg = NEXT_PAGE(pg);
  }
  return 1;
}

/*
 * mm_check - Check whether freeing the given `p`, which means that
 *            calling mm_free(p) leaves the heap in an ok state.
 */
int mm_can_free(void *p)
{
  //ensure pointer is mapped
  if( !ptr_is_mapped(p,GET_SIZE(HDRP(p))) )
    return 0;

  // ensure that block was allocated.
  if( GET_ALLOC(HDRP(p)) != 1 )
    return 0;


  return 1;
}
