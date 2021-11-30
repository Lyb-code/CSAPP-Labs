#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <math.h>
#define TRUE 1
#define FALSE 0

typedef int bool;
typedef struct cache_line
{
    int valid;
    int tag;
    int* block;
} cache_line_t;

cache_line_t **cache_memory;
int hits = 0, misses = 0, evictions = 0;
int set_bits = 0, associativity = 0, block_bits = 0;
bool help_flag = FALSE, verbose_flag = FALSE;
char* trace;

void getInputArguments(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    getInputArguments(argc, argv);
    printf("set_bits:%d associativity:%d block_bits:%d\n", set_bits, associativity, block_bits);
    printf("help_flag:%d verbose_flag:%d trace:%s\n", help_flag, verbose_flag, trace);
    // Allocate memory for cache
    int set_num = pow(2, set_bits);
    cache_memory = (cache_line_t **)calloc(set_num, sizeof(cache_line_t));// calloc can initialize allocated memory
    if (cache_memory == NULL) return 0;//calloc can fail
    for (int i = 0; i < set_num; i++) {
        cache_memory[i] = (cache_line_t *)calloc(associativity, sizeof(cache_line_t));
        if (cache_memory[i] == NULL) return 0;//calloc can fail
    }
    

    // free memory for cache
    for (int i = 0; i < set_num; i++) {
        free(cache_memory[i]);
    }
    free(cache_memory);

    printSummary(0, 0, 0);
    return 0;
}

/*
 * getInputArguments - Parse command line arguments
 */ 
void getInputArguments(int argc, char *argv[])
{   
    char opt;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt)
        {
        case 'h':
            help_flag = TRUE;
            break;
        case 'v':
            verbose_flag = TRUE;
            break;
        case 's':
            set_bits = atoi(optarg);
            break;
        case 'E':
            associativity = atoi(optarg);
            break;
        case 'b':
            block_bits = atoi(optarg);
            break;
        case 't':
            trace = optarg;
            break;
        default:
            break;
        }
    }
}