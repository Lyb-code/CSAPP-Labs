/*
 * Use Implicit List to organize blocks.
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
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

#define MAX(a, b) ((a) > (b) ? (a) : (b))
/* Constants */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)
/* Pack a size and allocated bit into a word */
#define PACK(size, allocated) ((size) | (allocated))
/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
/* Get size and allocated field at address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 1)
/* Operations of Block pointer bp */
#define HEADER(bp) ((char*)(bp) - WSIZE) 
#define FOOTER(bp) ((char*)(bp) + GET_SIZE(HEADER(bp)) - DSIZE)
#define NEXT_BLOCK(bp) ((char*)(bp) + GET_SIZE(HEADER(bp)))
#define PREV_BLOCK(bp) ((char*)(bp) - GET_SIZE((char*)(bp) - DSIZE))

static char* heap_listp;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    heap_listp = (char*)mem_sbrk(4 * WSIZE);
    if (heap_listp == ((void *)-1)) {
        return -1;
    }
    PUT(heap_listp, 0); //padding
    PUT(heap_listp + WSIZE, PACK(8, 1));//prologue block header
    PUT(heap_listp + 2 * WSIZE, PACK(8, 1));//prologue block footer
    PUT(heap_listp + 3 * WSIZE, PACK(0, 1));//epilogue block header
    heap_listp = heap_listp + 2 * WSIZE;
    
    if (extendHeap(CHUNKSIZE / WSIZE) == NULL) {//create a free block of CHUNKSIZE
        return -1;
    }

    return 0;
}

/* 
 * mm_malloc - Find a fit free block. If not, allocate a new free block.
 * param size: payload size
 */
void *mm_malloc(size_t size)
{
    if (size == 0) 
        return NULL;
    size_t newsize = size + 2 * WSIZE;//adjusted size
    newsize = (newsize + DSIZE - 1) & ~0x7;//alignment
    void *ptr = find_fit(newsize);
    if (ptr == NULL) 
        ptr = extendHeap(MAX(newsize, CHUNKSIZE) / WSIZE);
    if (ptr == NULL) 
        return NULL;
    use_block(ptr, newsize);
    return ptr;
}

/*
 * find a free block larger than "bsize" 
 * param bsize: block size(header+footer+payload+padding)
 */
static void *find_fit(size_t bsize)
{
    //return first_fit(bsize);
    return best_fit(bsize);
}

static void* first_fit(size_t bsize) {
    void *ptr = NEXT_BLOCK(heap_listp);
    while (GET_SIZE(HEADER(ptr)) != 0) { //when ptr is not epilogue block
        size_t ptr_size = GET_SIZE(HEADER(ptr));
        int alloc = GET_ALLOC(HEADER(ptr));
        if (ptr_size >= bsize && !alloc) {
            break;
        }
        ptr = NEXT_BLOCK(ptr);
    }
    return GET_SIZE(HEADER(ptr)) != 0 ? ptr : NULL;
}

static void *best_fit(size_t bsize) {
    void *ptr = NEXT_BLOCK(heap_listp);
    size_t ptr_size;
    void* best = NULL;
    size_t min_size = 0;

    while ((ptr_size = GET_SIZE(HEADER(ptr))) != 0) { //when ptr is not epilogue block
        int alloc = GET_ALLOC(HEADER(ptr));
        if (ptr_size >= bsize && !alloc && (min_size == 0 || min_size > ptr_size)) {
            min_size = ptr_size;
            best = ptr;
        }
        ptr = NEXT_BLOCK(ptr);
    }

    return best;
}

/* 
 * Use a free block to get space of a given "bsize".
 * Compare the size of a free block with given "bsize".If the difference 
 * exceeds the threshold, the free block will be split.Otherwise, the 
 * free block will be used directly.
 * param ptr: pointer to the payload of a free block
 * param bsize: block size(header+footer+payload+padding)
 */
static void use_block(void *ptr, size_t bsize) 
{
    size_t oldbsize = GET_SIZE(HEADER(ptr));
    if (oldbsize - bsize > DSIZE) {
        PUT(HEADER(ptr), PACK(bsize, 1));
        PUT(FOOTER(ptr), PACK(bsize, 1));
        PUT(HEADER(NEXT_BLOCK(ptr)), PACK(oldbsize - bsize, 0));
        PUT(FOOTER(NEXT_BLOCK(ptr)), PACK(oldbsize - bsize, 0));
    } else {
        PUT(HEADER(ptr), PACK(oldbsize, 1));
        PUT(FOOTER(ptr), PACK(oldbsize, 1));
    }
} 

/*
 * mm_free - Freeing a block and coalscue adjacent block.
 * param ptr: pointer to the payload of a allocated block
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HEADER(ptr));
    PUT(HEADER(ptr), PACK(size, 0));
    PUT(FOOTER(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * coalesce - coalesce adjacent free blocks.
 * param ptr: pointer to the payload of a free block
 */
static void *coalesce(void *ptr) 
{
    void *prev_p = PREV_BLOCK(ptr);
    void *next_p = NEXT_BLOCK(ptr);
    int prev_alloc = GET_ALLOC(HEADER(prev_p));
    int next_alloc = GET_ALLOC(HEADER(next_p));
    size_t combined_size = GET_SIZE(HEADER(ptr));
    if (prev_alloc && next_alloc) {
        return ptr;
    } else if (!prev_alloc && next_alloc) {
        combined_size += GET_SIZE(HEADER(prev_p));
        PUT(FOOTER(prev_p), 0);
        PUT(HEADER(prev_p), PACK(combined_size, 0));
        PUT(FOOTER(ptr), PACK(combined_size, 0));
        PUT(HEADER(ptr), 0);
        ptr = prev_p;
    } else if (prev_alloc && !next_alloc) {
        combined_size += GET_SIZE(HEADER(next_p));
        PUT(FOOTER(next_p), PACK(combined_size, 0));
        PUT(HEADER(next_p), 0);
        PUT(FOOTER(ptr), 0);
        PUT(HEADER(ptr), PACK(combined_size, 0));
    } else {
        combined_size += GET_SIZE(HEADER(prev_p)) + GET_SIZE(HEADER(next_p));
        PUT(FOOTER(next_p), PACK(combined_size, 0));
        PUT(HEADER(next_p), 0);
        PUT(FOOTER(ptr), 0);
        PUT(HEADER(ptr), 0);
        PUT(FOOTER(prev_p), 0);
        PUT(HEADER(prev_p), PACK(combined_size, 0));
        ptr = prev_p;
    }
    return ptr;
}

/*
 * mm_realloc - Implemented by document semantics
 * param ptr: pointer to the payload of a allocated block
 * param size: new paload size
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    if (ptr == NULL)
        newptr = mm_malloc(size);
    else if (size == 0) {
        mm_free(oldptr);
        newptr = oldptr;
    } else {
        long oldbsize = GET_SIZE(HEADER(oldptr));//bsize = block size
        long needbsize = (size + DSIZE + DSIZE - 1) & (~0x7);
        long difference = oldbsize - needbsize;
        if (difference > DSIZE) {
            PUT(HEADER(oldptr), PACK(needbsize, 1));
            PUT(FOOTER(oldptr), PACK(needbsize, 1));
            PUT(HEADER(NEXT_BLOCK(oldptr)), PACK(difference, 0));
            PUT(FOOTER(NEXT_BLOCK(oldptr)), PACK(difference, 0));
            coalesce(NEXT_BLOCK(oldptr));
            //memset((char*)oldptr, 0, needbsize - DSIZE - size);
            newptr = oldptr;
        } else {
            newptr = mm_malloc(size);
            if (newptr == NULL)
                return NULL;
            size_t copySize = oldbsize - DSIZE;//payload size
            copySize = copySize < size ? copySize : size;
            memcpy(newptr, oldptr, copySize);
            mm_free(oldptr);
        }
    }
    return newptr;
}

/*
 * Extend the heap by creating a free block of a given number of words
 */
static void *extendHeap(size_t words)
{
    words = words % 2 == 0 ? words : words + 1;
    size_t size = words * WSIZE;
    void *new_bp;
    if ((new_bp = mem_sbrk(size)) == ((void *)-1)) {
        return NULL;
    }
    PUT(HEADER(new_bp), PACK(size, 0));//overwirte epilogue header
    PUT(FOOTER(new_bp), PACK(size, 0));
    PUT(HEADER(NEXT_BLOCK(new_bp)), PACK(0, 1));//restore epilogue header
    /* Coalesce if previous block was free */
    return coalesce(new_bp);
}












