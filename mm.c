/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * You __MUST__ add your user information here and in the team array bellow.
 * 
 * === User information ===
 * Group: NONE
 * User 1: dude10
 * SSN: 1807825919
 * User 2: 
 * SSN: X
 * === End User Information ===
 */

/*
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
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

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Bananar",
    /* First member's full name */
    "Fannar Már Flosason",
    /* First member's email address */
    "fannarf12@ru.is",
    /* Second member's full name (leave blank if none) */
    "Þorsteinn Þorri Sigurðsson",
    /* Second member's email address (leave blank if none) */
    "thorsteinnts09@ru.is",
    /* Third member's full name (leave blank if none) */
    "",
    /* Third member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4			/* Word and header/footer size (bytes) */
#define DSIZE 8			/* Double word size (bytes) */
#define CHUNKSIZE (1<<12)	/* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) 		(*(unsigned int *)(p))
#define PUT(p, val) 	(*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) 	(GET(p) & ~0x7) /* Mask out the bottom 3 bits */
#define GET_ALLOC(p) 	(GET(p) & 0x1)  /* Mask out all but the lowest bit */

/* Given block ptr bp, compute address of its header and footer */
/*
 * 
 */
#define HDRP(bp)	((char *)(bp) - WSIZE)
#define FTRP(bp)	((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* ptr that points to the prologue block of the heap */
static char *heap_listp;

static void *mm_extend_heap(size_t words);
static void *mm_coalesce(void *bp);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    	
	if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
		return -1;
	
	PUT(heap_listp, 0);				/* Alignment padding */
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); 	/* Prologue header */
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));	/* Prologue footer  */
	PUT(heap_listp + (3*WSIZE), PACK(0, 1));	/* Epilogue header */
	heap_listp += (2*WSIZE);

	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if (mm_extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;	

	return 0;
}

/*
 * mm_extend_heap - Extends the heap with a new free block.
 */
static void *mm_extend_heap(size_t words)
{
	char *bp;
	size_t size;

	/* Allocate an even number of words to maintain alignment */
	size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;

	/* Initialize free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0));		/* Free block header */
	PUT(FTRP(bp), PACK(size, 0));		/* Free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));	/* New epilogue header */

	/* Coalesce if the previous block was free */
	return mm_coalesce(bp);
}

/*
 * mm_coalesce - In case of adjacent free blocks, merge them and change
 * 	headers and footers accordingly.
 */
static void *mm_coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); /* Checks if previous block is allocated */
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); /* Checks if next block is allocated */
	size_t size = GET_SIZE(HDRP(bp));

	/* TODO! For use in explicit free linked list:
	 * 	The predecessor pointer in the free block needs to be updated
	 * 	to the previous free block.
	 * 	The successor pointer in the free block needs to be updated
	 * 	to the next free block.
	 * 	The address of the predecessor and successor pointers need to
	 * 	be updated in case of merger with the left adjacent block.
	 */
	
	/* Case 1 - No adjacent free blocks (Both allocated) */
	if (prev_alloc && next_alloc)
	{
		return bp;
	}

	/* Case 2 - Right adjacent block is free but not the left */
	else if (prev_alloc && !next_alloc)
	{
		/*
 		 * The size of the adjacent block is added to the current block.
 		 * The header and footer are given the new size and an unallocated attribute.
 		 */
		size += GET_SIZE(HDRP(NEXT_BLKP(bp))); 
		PUT(HDRP(bp), PACK(size, 0)); /* Header */
		PUT(FTRP(bp), PACK(size, 0)); /* Footer address is located with HDRP(bp) and size */
	}

	/* Case 3 - Left adjacent block is free but not the right */
	else if (!prev_alloc && next_alloc)
	{
		/*
 		 * The size of the adjacent block is added to the current block.
 		 * The header of the left adjacent block becomes the header of the
 		 * current block and is given the new size and is set as unallocated.
 		 * The footer is given the new size and is set as unallocated.
 		 * The current block pointer is set as left adjacent block pointer.
 		 */  
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0)); 
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); 
		bp = PREV_BLKP(bp);
	}

	/* Case 4 - Both left and right adjacent blocks are free */
	else
	{
		/*
		 * The size of both adjacent blocks are added to the current block size.
		 * The header of the left adjacent block becomes the header of the
		 * current block and is given the new size and is set as unallocated.
		 * The footer of the right adjacent block becomes the footer of the
		 * current block and is given new size and is set as unallocated.
		 * The current block pointer is set as the left adjacent block pointer.
		 */ 
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}

	return bp;		

}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














