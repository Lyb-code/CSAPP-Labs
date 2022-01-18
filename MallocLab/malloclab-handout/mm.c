/*
 * Use Segregated Explicit Lists to organize free blocks.
 * Use Implicit List to organize all blocks.
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
/* The address in the block where the predecessor and successor addresses are stored*/
#define PRED(bp) ((char*)(bp) + WSIZE)
#define SUCC(bp) ((char*)(bp))
/* Get address of predecessor and successor block*/
#define PRED_BLOCK(bp) GET(PRED(bp))
#define SUCC_BLOCK(bp) GET(SUCC(bp))

/* Pointer to payload of prologue block */
static char* heap_listp;
/* Pointer to explicit free list of first size class */
static char* seg_listsp;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    heap_listp = (char*)mem_sbrk(12 * WSIZE);
    if (heap_listp == ((void *)-1)) {
        return -1;
    }
    //Nine size classes
    PUT(heap_listp, NULL); //16~31
    PUT(heap_listp + WSIZE, NULL); //32~63
    PUT(heap_listp + 2 * WSIZE, NULL); //64~127
    PUT(heap_listp + 3 * WSIZE, NULL); //128~255
    PUT(heap_listp + 4 * WSIZE, NULL); //256~511
    PUT(heap_listp + 5 * WSIZE, NULL); //512~1023
    PUT(heap_listp + 6 * WSIZE, NULL); //1024~2047
    PUT(heap_listp + 7 * WSIZE, NULL); //2048~4095
    PUT(heap_listp + 8 * WSIZE, NULL); //4096~INF

    PUT(heap_listp + 9 * WSIZE, PACK(8, 1));//prologue block header
    PUT(heap_listp + 10 * WSIZE, PACK(8, 1));//prologue block footer
    PUT(heap_listp + 11 * WSIZE, PACK(0, 1));//epilogue block header

    seg_listsp = heap_listp;
    heap_listp = heap_listp + 10 * WSIZE;
    
    if (extendHeap(CHUNKSIZE / WSIZE) == NULL) {//create a free block of CHUNKSIZE
        return -1;
    }

    return 0;
}

/** 
 * mm_malloc - Find a fit free block. If not, allocate a new free block.
 * @param size: payload size
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

/**
 * find a free block larger than "bsize" 
 * @param bsize: block size(header+footer+payload+padding)
 */
static void *find_fit(size_t bsize)
{
    return first_fit(bsize);
    //return best_fit(bsize);
}

static void* first_fit(size_t bsize) {
    void *listp, *bp;
    int index = size_class(bsize);
    while (index <= 8) {
        listp = seg_listsp + index * WSIZE;
        bp = SUCC_BLOCK(listp);
        while (bp) {
            if (GET_SIZE(HEADER(bp)) >= bsize)
                return bp;
            bp = SUCC_BLOCK(bp);
        }
        index++;
    }
    return NULL;
}

static void *best_fit(size_t bsize) {
    void *listp, *bp, *best = NULL;
    size_t curr_size;
    size_t min_size = 0;
    int index = size_class(bsize);
    while (index <= 8) {
        listp = seg_listsp + index * WSIZE;
        bp = SUCC_BLOCK(listp);
        while (bp) {
            curr_size = GET_SIZE(HEADER(bp));
            if (curr_size >= bsize && (min_size == 0 || curr_size < min_size)) {
                min_size = curr_size;
                best = bp;
            }
            bp = SUCC_BLOCK(bp);
        }
        if (best) break;
        index++;
    }
    return best;
}

/** 
 * Use a free block to get space of a given "bsize".
 * Compare the size of a free block with given "bsize". If the difference 
 * exceeds the threshold, the free block will be split. Otherwise, the 
 * free block will be used directly.
 * @param bp: block pointer to the payload of a free block
 * @param bsize: block size(header+footer+payload+padding)
 */
static void use_block(void *bp, size_t bsize) 
{
    size_t oldbsize = GET_SIZE(HEADER(bp));
    delFreeBlock(bp);
    if (oldbsize - bsize > 2 * DSIZE) {
        PUT(HEADER(bp), PACK(bsize, 1));
        PUT(FOOTER(bp), PACK(bsize, 1));
        PUT(HEADER(NEXT_BLOCK(bp)), PACK(oldbsize - bsize, 0));
        PUT(FOOTER(NEXT_BLOCK(bp)), PACK(oldbsize - bsize, 0));
        addFreeBlock(NEXT_BLOCK(bp));
    } else {
        PUT(HEADER(bp), PACK(oldbsize, 1));
        PUT(FOOTER(bp), PACK(oldbsize, 1));
    }
} 

/**
 * mm_free - Freeing a block and coalscue adjacent block.
 * @param ptr: block pointer to the payload of a allocated block
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HEADER(ptr));
    PUT(HEADER(ptr), PACK(size, 0));
    PUT(FOOTER(ptr), PACK(size, 0));
    ptr = coalesce(ptr);
    addFreeBlock(ptr);
}

/**
 * coalesce - coalesce adjacent free blocks.
 * @param bp: block pointer to the payload of a free block
 */
static void *coalesce(void *bp) 
{
    void *prev_p = PREV_BLOCK(bp);
    void *next_p = NEXT_BLOCK(bp);
    int prev_alloc = GET_ALLOC(HEADER(prev_p));
    int next_alloc = GET_ALLOC(HEADER(next_p));
    size_t combined_size = GET_SIZE(HEADER(bp));
    if (prev_alloc && next_alloc) {
        return bp;
    } else if (!prev_alloc && next_alloc) {
        delFreeBlock(prev_p);
        combined_size += GET_SIZE(HEADER(prev_p));
        PUT(HEADER(prev_p), PACK(combined_size, 0));
        PUT(FOOTER(bp), PACK(combined_size, 0));
        bp = prev_p;
    } else if (prev_alloc && !next_alloc) {
        delFreeBlock(next_p);
        combined_size += GET_SIZE(HEADER(next_p));
        PUT(HEADER(bp), PACK(combined_size, 0));
        PUT(FOOTER(next_p), PACK(combined_size, 0));
    } else {
        delFreeBlock(prev_p);
        delFreeBlock(next_p);
        combined_size += GET_SIZE(HEADER(prev_p)) + GET_SIZE(HEADER(next_p));
        PUT(HEADER(prev_p), PACK(combined_size, 0));
        PUT(FOOTER(next_p), PACK(combined_size, 0));
        bp = prev_p;
    }
    return bp;
}

/**
 * mm_realloc - Implemented by document semantics
 * @param ptr: block pointer to the payload of a allocated block
 * @param size: new paload size
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
        if (difference > 2 * DSIZE) {
            PUT(HEADER(oldptr), PACK(needbsize, 1));
            PUT(FOOTER(oldptr), PACK(needbsize, 1));
            PUT(HEADER(NEXT_BLOCK(oldptr)), PACK(difference, 0));
            PUT(FOOTER(NEXT_BLOCK(oldptr)), PACK(difference, 0));
            addFreeBlock(coalesce(NEXT_BLOCK(oldptr)));
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
    PUT(PRED(new_bp), NULL);
    PUT(SUCC(new_bp), NULL);
    PUT(HEADER(NEXT_BLOCK(new_bp)), PACK(0, 1));//restore epilogue header
    /* Coalesce if previous block was free */
    new_bp = coalesce(new_bp);
    addFreeBlock(new_bp);
    return new_bp;
}

/**
 * Add a free block to the explicit list of corresponding size classes
 * @param bp: block pointer to the payload of a free block
 */
static void addFreeBlock(void *bp)
{
    size_t bsize = GET_SIZE(HEADER(bp));
    int index = size_class(bsize);
    char* listp = seg_listsp + index * WSIZE;
    LIFO(listp, bp);
    //AddressOrdered(listp, bp);
}

/**
 * Delete the free block from the explicit list of corresponding size classes
 * @param bp: block pointer to the payload of a free block
 */
static void delFreeBlock(void *bp)
{
    void *pred_block = PRED_BLOCK(bp);
    void *succ_block = SUCC_BLOCK(bp);
    PUT(SUCC(pred_block), succ_block);
    if (succ_block != NULL)
        PUT(PRED(succ_block), pred_block);
}

/**
 * Get size class of given "size".
 * Note that 16~31 correspond to class 0.
 */
static int size_class(size_t size)
{
    if (size >= 4096)
        return 8;
    int sc = 0;
    size >>= 5;
    while (size) {
        size >>= 1;
        sc++;
    }
    return sc;
}

/**
 * Insert a free block according to LIFO.
 * @param listp : pointer to the head of explicit free list 
 * @param bp : block pointer to the payload of a free block
 */
static void LIFO(void *listp, void *bp)
{
    void *first_block = SUCC_BLOCK(listp);
    //bp <--> first_block
    PUT(SUCC(bp), first_block);
    if (first_block != NULL) {
        PUT(PRED(first_block), bp);    
    }
    //listp <--> bp
    PUT(SUCC(listp), bp);
    PUT(PRED(bp), listp);
}

/**
 * Insert a free block according to AddressOrdered policy.
 * @param listp : pointer to the head of explicit free list 
 * @param bp : block pointer to the payload of a free block
 */
static void AddressOrdered(void *listp, void *bp)
{
    void *pred_block = listp, *succ_block = SUCC_BLOCK(listp);
    while(succ_block) {
        if (succ_block > bp) {
            break;
        }
        pred_block = succ_block;
        succ_block = SUCC_BLOCK(succ_block);
    }

    //pred_block <--> bp
    PUT(SUCC(pred_block), bp);
    PUT(PRED(bp), pred_block);
    //bp <--> succ_block
    PUT(SUCC(bp), succ_block);
    if (succ_block != NULL) 
        PUT(PRED(succ_block), bp);
}








