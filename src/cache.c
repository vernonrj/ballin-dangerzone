#include "cache.h"
#include <errno.h>


/* 
 *
 */
struct cache* cache_new(size_t associativity, size_t line_size, size_t index_size)
{
    return NULL;
}
// LRU functions

void cache_free(struct cache* cacheobj)
{
    //stub
}

uint32_t touchLRU(struct set_t *set, uint32_t way)
{
	// update LRU 
	int i;
	uint32_t pivot = set->lru[way];

	for (i=0; i<set->linesize; i++)
	{
		if (set->lru[i] < pivot)
			set->lru[i]++;
		else if (set->lru[i] == pivot)
			set->lru[i] = 0x0;
	}

	return 0;
}


uint32_t findLRU(struct set_t *set)
{
	// return the least recently used way in a set
	int i;

	for (i=0; i<set->linesize; i++)
	{
		if (set->lru[i] == (set->linesize - 1))
			return i;
	}
	return 0;
}


void invalidateLRUWay(struct set_t *set, uint32_t way)
{
	// set LRU way to least recently used

	int i;
	uint32_t pivot = set->lru[way];

	for (i=0; i<set->linesize; i++)
	{
		if (i == way)
			set->lru[i] = set->linesize - 1;
		else if (set->lru[i] > pivot)
			set->lru[i]--;
	}

	return;
}


// Address Decomposition

uint32_t getTag(uint32_t address, uint32_t indexsize, uint32_t offsetsize)
{
	// return the tag, right-justified

	return address >> (offsetsize+indexsize);
}


uint32_t getIndex(uint32_t address, uint32_t indexsize, uint32_t offsetsize)
{
	// return the index, right-justified

	return (address >> offsetsize) & ((0x1 << indexsize) - 1);
}


uint32_t getOffset(uint32_t address, uint32_t indexsize, uint32_t offsetsize)
{
	// return the byte offset, right-justified

	return (address & ((0x1 << offsetsize) - 1));
}


// Checking

bool lineIsValid(struct line_t *line)
{
	// check validity of line

	return ((line->status & 3) != MESI_INVALID);
}


// Finding

int findWithTag(struct set_t *set, uint32_t tag)
{
	// return way that matches tag
	// return -1 if cache miss
	// don't touch LRU order

	int i;
	int size = set->linesize;
	struct line_t **line = set->lines;

	for (i=0; i<size; i++)
	{
		if ((lineIsValid(line[i]) && line[i]->tag == tag))
			return i;
	}

	return -1;
}



// Status

void printStats(struct cache_t *cache, const char *name)
{
	// print statistics about the cache
	printf("\n%s:\n", name);
	printf("Reads: %i\n", cache->c_reads);
	printf("Writes: %i\n", cache->c_writes);
	printf("Hits: %i\n", cache->c_hits);
	printf("Misses: %i\n", cache->c_misses);

	return;
}



int main(int argc, char **argv)
{
	int i;
	char *line;
	FILE *fptr;// = openFile(path);
	int state;
	uint32_t command, address;
#ifdef DEBUG
	VERBOSITY = MAX_VERBOSITY;
#else
	VERBOSITY = DEFAULT_VERBOSITY;
#endif

	if (argc > 1)
	{
		for (i=0; i<argc-1; i++)
		{
			if (!strcmp(argv[i], "--verbose"))
				VERBOSITY = 1;
			else if (!strcmp(argv[i], "--more-verbose"))
				VERBOSITY = 2;
			else if (!strcmp(argv[i], "--quiet"))
				VERBOSITY = 0;
		}
		fptr = openFile(argv[argc-1]);
	}
	else
	{
		printf("Please specify a file.\n");
		exit(EXIT_FAILURE);
	}

	// generate an L2 cache (handlers are stubs)
	L2 = cacheInit(CACHE_L2, 1);
	// generate L1 caches that link to a unified L2 cache
	L1I = cacheInit(CACHE_L1, 2);
	L1D = cacheInit(CACHE_L1, 4);
	if (VERBOSITY > 0)
		printf("Status\t\tAddress\t\tMESI\tTag\tIndex\tLine\tOffset\tValue\n");


	line = (char*)malloc(500*sizeof(char));
	while ((state = parseNextLine(fptr, line, 500, &command, &address)) != -1)
	{
		if (state == 1)
			continue;
		switch (command)
		{
			case COMMAND_READ:
				L1Read(L1D, address);
				break;
			case COMMAND_WRITE:
				L1Write(L1D, address, address);
				break;
			case COMMAND_IFETCH:
				L1Read(L1I, address);
				break;
			case COMMAND_INVALIDATE:
				L2Invalidate(L2, address);
				break;
			case COMMAND_RESET:
				L2Reset(L2);
				break;
			case COMMAND_PRINT:
				L1PrintCache(L1D);
				L1PrintCache(L1I);
				break;
			default:
				printf("parsing error\n");
		}
	}
	free(line);
	fclose(fptr);


	printStats(L1I, "L1 Instruction Cache");

	printStats(L1D, "L1 Data Cache");

	L1PrintCache(L1D);

	cache_destroy(L1D);
	cache_destroy(L1I);

	return 0;
}


struct cache_t *cache_destroy(struct cache_t *cache)
{
	int i;
	for (i=0; i<cache->setsize; i++)
		set_destroy(cache->sets[i]);
	free(cache);
	return NULL;
}

void set_destroy(struct set_t *set)
{
	int i;
	for (i=0; i<set->linesize; i++)
	{
		if (set->lines[i]->data)
			free(set->lines[i]->data);
		free(set->lines[i]);
	}
	free(set->lines);
	free(set->lru);
	free(set);
	return;
}

