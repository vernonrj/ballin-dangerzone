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

// Global Definitions
#define LINESIZE 16	// 64 byte lines
			// using 32-bit integers

#define L2USESHARED 0

int VERBOSITY;
// Data Structures

enum Command_enum
{
	COMMAND_READ,
	COMMAND_WRITE,
	COMMAND_IFETCH,		// read request from L1
	COMMAND_INVALIDATE,	// comes from L2
	COMMAND_RESET,	
	COMMAND_PRINT
};
	
enum Level_enum 
{
	// specify cache level
	CACHE_L1, CACHE_L2
};

enum Mesi_enum
{
	// specify mesi states
	MESI_MODIFIED, MESI_EXCLUSIVE, MESI_SHARED, MESI_INVALID
};

enum Cache_Write_Type
{
	WRITE_MODIFY, WRITE_ALLOCATE
};

struct line_t
{
	// line structure
	uint32_t tag;			// cache tag
	uint32_t *data;	// line data
	uint32_t status;		// MESI protocol
		// 00	= Invalid
		// 10	= Shared
		// 01	= Exclusive
		// 11	= Modified
};

struct set_t
{
	// individual set structure
	uint32_t *lru;			// LRU bits:
					// N*log2(N)
	int linesize;			// number of lines per set
	struct line_t **lines;		// lines in set
	//struct line_t lines[];		// lines in set
       	//int lrusize;			// number of bits in LRU
};


struct LN_ops_t
{
	// address, data size, data
	void (*read)(uint32_t, size_t, uint32_t*);
	void (*write)(uint32_t, size_t, uint32_t*);
};

struct cache_t
{
	// cache data structure
	// TODO: declare set_t as 0-length array
	// then malloc(sizeof(cache_t)+sizeof(N*set_t));
	// declare as set_t[0]
	int setsize;			// number of sets
	int c_reads,			// cache statistics
	    c_writes,
	    c_hits,
	    c_misses;
	//struct set_t **sets;		// array of sets
	struct LN_ops_t lnops;
	struct set_t *sets[];
};

struct cache_t *L1I, *L1D, *L2;

// parser.c
FILE *openFile(char *path);
int parseNextLine(FILE *fd, char *line, int lineLength, uint32_t *command_t, uint32_t *address_t);


// l1cache.c
// L1 cache functions
bool setCheckTagExists(struct set_t **sets, uint32_t index, uint32_t tag);
bool lineInvalNextIfShared(struct cache_t *cache, struct line_t *line, uint32_t address);
uint32_t *setRead(struct set_t *set, uint32_t tag);
//void setWriteWord(struct cache_t *cache, uint32_t address, uint32_t data, enum Cache_Write_Type writeType);
void setWrite(struct cache_t *cache, uint32_t address, uint32_t *data, enum Cache_Write_Type writeType);
void L1Reset(struct cache_t *cache);
uint32_t *L1Read(struct cache_t *cache, uint32_t address);
void L1WriteLine(struct cache_t *cache, uint32_t address, uint32_t *data, enum Cache_Write_Type writeType);
void L1Write(struct cache_t *cache, uint32_t address, uint32_t data);
void L1Invalidate(struct cache_t *cache, uint32_t address);
void L1PrintLine(struct cache_t *cache, uint32_t address);
void L1PrintCache(struct cache_t *cache);
bool L1CheckAddress(struct cache_t *cache, uint32_t address);


// l2cache.c
// L2 cache functions
void L2Reset(struct cache_t *cache);
uint32_t *L2Read(struct cache_t *cache, uint32_t address);
void L2Write(struct cache_t *cache, uint32_t address, uint32_t data);
void L2Invalidate(struct cache_t *cache, uint32_t address);
void L2WriteLine(struct cache_t *cache, uint32_t address, uint32_t *data, enum Cache_Write_Type);
bool L2CheckAddress(struct cache_t *cache, uint32_t address);


// cache.c
// general cache functions
// initialization functions
struct set_t *setInit(int linesize);
struct cache_t *cacheInit(enum Level_enum level, int linesize);
// LRU functions
uint32_t touchLRU(struct set_t *set, uint32_t way);
uint32_t findLRU(struct set_t *set);
void invalidateLRUWay(struct set_t *set, uint32_t way);
// address decomposition
uint32_t getTag(uint32_t address, uint32_t indexsize, uint32_t offsetsize);
uint32_t getIndex(uint32_t address, uint32_t indexsize, uint32_t offsetsize);
uint32_t getOffset(uint32_t address, uint32_t indexsize, uint32_t offsetsize);
// checking
bool lineIsValid(struct line_t *line);
// finding
int findWithTag(struct set_t *set, uint32_t tag);
// generic read/write: calls cache handlers

void printStats(struct cache_t *cache, const char *name);
int main(int argc, char **argv);
struct cache_t *cache_destroy(struct cache_t *cache);
void set_destroy(struct set_t *set);

