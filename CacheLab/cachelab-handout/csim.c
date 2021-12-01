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
    unsigned long tag;
    int access_time;
    //Just simulate the cache, no need to actually allocate block
} cache_line_t;

cache_line_t **cache_memory;
int hits_num = 0, misses_num = 0, evictions_num = 0;
int set_bits = 0, associativity = 0, block_bits = 0;
int access_time = 0;
bool help_flag = FALSE, verbose_flag = FALSE;
char* traceFileName;

void getInputArguments(int argc, char *argv[]);
void searchCacheLine(int set_idx, unsigned long tag);
void printHelpInfo();

int main(int argc, char *argv[])
{
    getInputArguments(argc, argv);
    if (help_flag) {
        printHelpInfo();
        return 0;
    }
    // Allocate memory for cache
    int set_num = pow(2, set_bits);
    cache_memory = (cache_line_t **)calloc(set_num, sizeof(cache_line_t *));// calloc can initialize allocated memory
    if (cache_memory == NULL) return 0;//calloc can fail
    for (int i = 0; i < set_num; i++) {
        cache_memory[i] = (cache_line_t *)calloc(associativity, sizeof(cache_line_t));
        if (cache_memory[i] == NULL) return 0;//calloc can fail
    }

    // read trace files
    FILE *tFile;
    tFile = fopen(traceFileName, "r"); //open file for reading
    char access_type;
    unsigned long address;
    int size;
    while (fscanf(tFile, " %c %lx,%d", &access_type, &address, &size) > 0) {
        if (access_type == 'I')continue;
        if (verbose_flag) {
            printf("%c %lx,%d", access_type, address, size);
        }
        // parse address
        unsigned long full_mask = -1;//111...1
        unsigned long set_mask = full_mask >> (64 - set_bits);
        unsigned long tag = address >> (block_bits + set_bits);
        int set_idx = (address >> block_bits) & set_mask;
        // search cache line
        searchCacheLine(set_idx, tag);
        if (access_type == 'M') { // M(modift) = L(load) + S(store). search twice
            searchCacheLine(set_idx, tag);
        }
        if (verbose_flag) {
            printf("\n");
        }
    }
    fclose(tFile);// clean up resources

    // free memory for cache
    for (int i = 0; i < set_num; i++) {
        free(cache_memory[i]);
    }
    free(cache_memory);

    printSummary(hits_num, misses_num, evictions_num);
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
            traceFileName = optarg;
            break;
        default:
            break;
        }
    }
}

/*
 * search cache line
 */
void searchCacheLine(int set_idx, unsigned long tag) {
    bool hit_flag = FALSE;
    for (int j = 0; j < associativity; j++) { // find hit line
        cache_line_t *line = &cache_memory[set_idx][j];
        if (line->valid && line->tag == tag) { // hit line
            if (verbose_flag) {
                printf(" hit");
            }
            hit_flag = TRUE;
            line->access_time = access_time++;
            hits_num++;
            break;
        }
    }
    if (!hit_flag) { // find invalid line and eviction line
        if (verbose_flag) {
            printf(" miss");
        }
        misses_num++;
        bool invalid_found = FALSE;
        for (int j = 0; j < associativity; j++) { 
            cache_line_t *line = &cache_memory[set_idx][j];
            if (!line->valid) { // invalid line
                if (!invalid_found) {
                    invalid_found = TRUE;
                    line->valid = 1;
                    line->tag = tag;
                    line->access_time = access_time++;
                }
                break;
            } 
        }
        if (!invalid_found) {
            if (verbose_flag) {
                printf(" eviction");
            }
            cache_line_t *eviction_line = &cache_memory[set_idx][0];
            for (int j = 1; j < associativity; j++) { 
                cache_line_t *line = &cache_memory[set_idx][j];
                if (line->access_time < eviction_line->access_time) { // eviction line
                    eviction_line = line;
                } 
            }
            eviction_line->tag = tag;
            eviction_line->access_time = access_time++;
            evictions_num++;
        }
    }
}

void printHelpInfo() {
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n\n");
    printf("Examples:\n");
    printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}