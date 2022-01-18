#include <stdio.h>

extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
extern void *mm_realloc(void *ptr, size_t size);
static void *find_fit(size_t bsize);
static void *first_fit(size_t bsize);
static void *best_fit(size_t bsize);
static void use_block(void *ptr, size_t bsize);
static void *extendHeap(size_t words);
static void addFreeBlock(void *bp);
static void delFreeBlock(void *bp);
static int size_class(size_t size);
static void LIFO(void *listp, void *bp);
static void AddressOrdered(void *listp, void *bp);
static void *coalesce(void *bp);

/* 
 * Students work in teams of one or two.  Teams enter their team name, 
 * personal names and login IDs in a struct of this
 * type in their bits.c file.
 */
typedef struct {
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *id1;      /* login ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *id2;      /* login ID of second member */
} team_t;

extern team_t team;

