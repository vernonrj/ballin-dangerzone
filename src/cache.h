#ifndef __CACHE_H__
#define __CACHE_H__

// L1 Cache:
// index = 14
// offset = 6
// tag = 12
// L1 Instruction Cache has 2 lines per set
// L1 Data Cache has 4 lines per set

// L2 Cache: Stub
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Compile-time switches
//#define DEBUG
//#define VERBOSE_PRINT_
#define DEFAULT_VERBOSITY 0
#define MAX_VERBOSITY 3

int VERBOSITY;
// Data Structures

enum Command_enum
{
	COMMAND_READ 		= 0,
	COMMAND_WRITE 		= 1,
	COMMAND_IFETCH 		= 2,		// read request from L1
	COMMAND_INVALIDATE 	= 3,	// comes from L2
	COMMAND_RESET 		= 8,	
	COMMAND_PRINT 		= 9
};
	
// Line Structure
struct line_t
{
    uint32_t tag;               // cache tag
    uint8_t status;
    size_t  data_size;
    uint8_t data[];	        // line data
};

// Set structure
struct set_t
{
    uint32_t *lru;			// LRU bits:
    int lrusize;			// number of bits in LRU
					// Space complexity: N*log2(N)
    size_t linesize;			// number of lines per set
    struct line_t *line[];		// lines in set

};

// Lower Memory hierarchy callback functions
struct LN_ops_t
{
	void (*read)(uint32_t address, size_t length, uint8_t* data);
	void (*write)(uint32_t address, size_t length, uint8_t* data);
};


struct cache_t
{
    // cache data structure
    int setsize;     // number of sets
    int c_reads,     // cache statistics
	c_writes,
	c_hits,
	c_misses;
    struct LN_ops_t ln_ops;     //callbacks to the lower memory hierarchy
    struct set_t *set[];        //
};

//**************** cache api *************************************************//
/* line size  = logn bytes in a line
 * index size = logn sets  
 */

struct cache* cache_new(size_t associativity, size_t line_size, size_t index_size);
void cache_free(struct cache* cacheobj);

// input operations
void cache_readop(struct cache* cacheobj, uint32_t address);
void cache_writeop(struct cache* cacheobj, uint32_t address);
void cache_invalidate(struct cache* cacheobj, uint32_t address);

// debuging functions
void cache_print(const struct cache* cacheobj);


/*
// l2cache.c
// L2 cache functions
uint32_t *read_stub(struct cache_t *cache, uint32_t address, size_t line_length, uint8_t* data);
void write_stub(struct cache_t *cache, uint32_t address, size_t line_length, uint8_t* data);
*/

#endif //__CACHE_H__

