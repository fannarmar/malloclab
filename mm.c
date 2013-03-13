/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * You __MUST__ add your user information here and in the team array bellow.
 * 
 * === User information ===
 * Group: GroupGroup
 * User 1: fannarf12
 * SSN: 2709892339
 * User 2: thorsteinnts09
 * SSN: 2903892169
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
    "GroupGroup",
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
#define CHUNKSIZE (1<<10)	/* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) 		(*(unsigned int *)(p))
#define PUT(p, val) 	(*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) 	(GET(p) & ~0x7) /* Mask out the bottom 3 bits */
#define GET_ALLOC(p) 	(GET(p) & 0x1)  /* Mask out all but the lowest bit */

/* End of free list attributes. Second left most bit: Last free block. Third left most bit: First free block */
#define SET_FIRST_FREE(p)	(GET(p) | 0x4)
#define SET_LAST_FREE(p)	(GET(p) | 0x2)

#define SET_NOT_FIRST_FREE(p)	(GET(p) & ~0x4)
#define SET_NOT_LAST_FREE(p)	(GET(p) & ~0x2)

#define GET_FIRST_FREE		((GET(p) & 0x4) >> 2)
#define GET_LAST_FREE		((GET(p) & 0x2) >> 1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)	((char *)(bp) - WSIZE)
#define FTRP(bp)	((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given free block ptr bp, compute address of its predecessor and successor */
#define PRED(bp)	((char *)(HDRP(bp) + (1*WSIZE)))
#define SUCC(bp)	((char *)((HDRP(bp)) + (2*WSIZE)))

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


/* Helper macros for testing */
#define PRINT_ROOT_POINTER(bp)	(printf("heap_listp: %p\n", (char *)(bp)))

static char *rootp; /* ptr that points to the root of the free list. */
static char *endp = NULL; /* ptr that points to the last block in the free list. */

static void *mm_extend_heap(size_t words);
static void *mm_coalesce(void *bp);
static void *mm_find_fit(size_t adjusted_size);
static void mm_place(void *bp, size_t adjusted_size);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{  

	/* Initialize rootp and endp as 0 */
	rootp = 0;
	endp = 0;
 
	/* mem_sbrk increments the heap and returns the old pointer */	
	if ((rootp = mem_sbrk(4*WSIZE)) == (void *)-1)
		return -1;
	
	PUT(rootp, 0);				/* Alignment padding */
	PUT(rootp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */
	PUT(rootp + (2*WSIZE), PACK(DSIZE, 1));	/* Prologue footer  */
	PUT(rootp + (3*WSIZE), PACK(0, 1));	/* Epilogue header */
	
	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
 	if ((rootp = mm_extend_heap(CHUNKSIZE/WSIZE)) == NULL)
		return -1;	


	//rootp += (4*WSIZE); /* rootp points to the payload of the first free block */
	endp = rootp; /* endp points to the payload of the last free block (first == last) */

	/* INIT DEBUG */

	printf("Low heap: %p \n", mem_heap_lo());	
	printf("rootp: %p \n", rootp);	
	printf("endp: %p \n", endp);

	printf("Epilogue footer: %p \n", HDRP(rootp) - WSIZE);
	printf("Epilogue footer (data): %d \n", *(HDRP(rootp) - WSIZE));	

	printf("FreeHeapHeader: %p \n", HDRP(rootp));	
	printf("FreeHeapHeader (data): 0x%x \n", *(unsigned int *)HDRP(rootp));	
	printf("endp: %p \n", endp);

	printf("Predecessor: %p \n", PRED(rootp));	
	printf("Predecessor (data): %d \n", *(PRED(rootp)));	
	printf("Successor: %p \n", SUCC(rootp));	
	printf("Successor (data): %d \n", *(SUCC(rootp)));	
	
	printf("FreeHeapFooter: %p \n", FTRP(rootp));	
	printf("FreeHeapFooter (Data): 0x%x \n", *(unsigned int *)FTRP(rootp));		

	printf("END OF INIT \n");
	printf("\n");

	
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

	/* Set predeccessor and successor pointers */
	if (endp == NULL)
		PUT(PRED(bp), 0); /* No free block in list */
	else
		PUT(PRED(bp), *endp); /* endp points to last free block */
	
	PUT(SUCC(bp), 0);  /* Successor - is last free block */

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
 * mm_find_fit - Finds the first free block with sufficient size.
 * 	Returns NULL if no free block is big enough.
 */
static void *mm_find_fit(size_t adjusted_size)
{	
	/* 
	 * pointer that jumps between free blocks until a sufficient block
	 * is found.
	 */
	char *runner = rootp; /* Root of free list */

	/* mm_find_fit DEBUG */
	
	printf("runner pointer: %p \n", runner);
	printf("Current free block Size: %d \n", GET_SIZE(HDRP(runner)));
	
	while (GET_SIZE(HDRP(runner)) < adjusted_size)
	{
		if (runner == 0)
			return NULL;
		runner = (char *)(SUCC(runner)); /* points to next free block */
	}
	
	return runner;
}

/*
 * mm_place - Places an allocated block inside the free list.
 */
static void mm_place(void *bp, size_t adjusted_size)
{
	char *prev_bp; 			/* pointer to payload of previous free block */
	char *next_bp;
	char *new_freeblock_bp;
	char *current_predecessor_p;
	char *current_successor_p;
	size_t current_freeblock_size;
	size_t new_freeblock_size;

	/* Current predecessor and successor of the 
	 * free block about to be allocated */
	current_predecessor_p = PRED(bp);
	current_successor_p = SUCC(bp);
	current_freeblock_size = GET_SIZE(HDRP(bp));
	new_freeblock_size = current_freeblock_size - adjusted_size;

	/* Allocate memory */
	PUT(HDRP(bp), PACK(adjusted_size, 1)); /* Allocated block header */
	PUT(FTRP(bp), PACK(adjusted_size, 1)); /* Allocated block footer */
	
	/* Adjust new free block */
	new_freeblock_bp = NEXT_BLKP(bp);
	PUT(HDRP(new_freeblock_bp), PACK(new_freeblock_size, 0));
	PUT(FTRP(new_freeblock_bp), PACK(new_freeblock_size, 0));
	PUT(PRED(new_freeblock_bp), *current_predecessor_p);
	PUT(SUCC(new_freeblock_bp), *current_successor_p); 
	
	/* Adjust previous and next free blocks to point to adjusted free block */
	if (!GET_SIZE(current_predecessor_p))
	{	
		prev_bp = (char *)current_predecessor_p;
		PUT(SUCC(prev_bp),*(unsigned int *)new_freeblock_bp);
	}
	if (!GET_SIZE(current_successor_p))
	{
		next_bp = (char *)current_successor_p;
		PUT(PRED(next_bp),*(unsigned int *)new_freeblock_bp);	
	}

	/* Increments runner if new free block is the last free block */
	if (bp == rootp)
		rootp = new_freeblock_bp;
	

	/* mm_place - DEBUG */
	printf("New malloced block size: %d \n", adjusted_size);
	printf("Malloc Header: %p \n", HDRP(bp));
	printf("Malloc Header (data): %d \n", *HDRP(bp));
	printf("Malloc Footer: %p \n", FTRP(bp));
	printf("Malloc Footer (data): %d \n", *FTRP(bp));

	printf("\n");	

	printf("Adjusted Free Heap, size: %d \n", new_freeblock_size);

	printf("Adjusted Free Heap Header: %p \n", HDRP(new_freeblock_bp));	
	printf("Adjusted Free Heap Header (data): %d \n", GET_SIZE(HDRP(new_freeblock_bp)));
	printf("Adjusted Free Heap Footer: %p \n", FTRP(new_freeblock_bp));	
	printf("Adjusted Free Heap Footer (data): %d \n", GET_SIZE(FTRP(new_freeblock_bp)));	
	

	
	printf("END OF PLACE\n");
	printf("\n");
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	/* New block size adjusted to next 8 byte block size + 8 bytes for header and footer */
	size_t adjusted_size = ALIGN(size + SIZE_T_SIZE);
	char *bp;
	size_t words;

	/* MALLOC DEBUG */
	printf("Malloced size: %d \n", adjusted_size);	


	/* Ignore spurious requests */
	if (size == 0)
		return NULL;

	/* Search for available free block with appropriate size */
	if ((bp = mm_find_fit(adjusted_size)) != NULL)
	{
		/* Place block inside free heap */
		mm_place(bp, adjusted_size);
		return bp;
	}
	else
	{	
		words = adjusted_size > CHUNKSIZE ? (adjusted_size + CHUNKSIZE)/WSIZE : CHUNKSIZE/WSIZE;
		bp = mm_extend_heap(words);
		mm_place(bp, adjusted_size);
		return bp;
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














