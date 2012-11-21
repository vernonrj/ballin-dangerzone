// L1 cache functions
#include "cache.h"

#define INDEXSIZE 14	// 14-bit indexes
#define OFFSETSIZE 4	// 4-bit offset
#define TAGSIZE (32 - INDEXSIZE - OFFSETSIZE)


// Wrappers for decomposing the address into the tag, the index, and the offset

uint32_t getL1Wrapper(uint32_t address, uint32_t (*fptr)(uint32_t, uint32_t, uint32_t))
{
	// call function given by fptr with appropriate arguments

	return fptr(address, INDEXSIZE, OFFSETSIZE);
}

uint32_t getL1Tag(uint32_t address)
{
	// wrapper for getting the tag of an L1 cache

	//return getL1Wrapper(address, &getTag);
	return address >> (OFFSETSIZE+INDEXSIZE);
}


uint32_t getL1Index(uint32_t address)
{
	// wrapper for getting the index of an L1 cache

	//return getL1Wrapper(address, &getIndex);
	return (address >> OFFSETSIZE) & ((0x1 << INDEXSIZE) -1);
}


uint32_t getL1Offset(uint32_t address)
{
	// wrapper for getting the offset of an L1 cache

	//return getL1Wrapper(address, &getOffset);
	return (address & ((0x1 << OFFSETSIZE) - 1));
}


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
	struct line_t **line = set->line;

	for (i=0; i<size; i++)
	{
		if ((lineIsValid(line[i]) && line[i]->tag == tag))
			return i;
	}

	return -1;
}


// LRU functions

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



// set-specific functions

bool setCheckTagExists(struct set_t **sets, uint32_t index, uint32_t tag)
{
	// check to see if tag exists at index
	// true:	tag exists
	// false:	tag is missing
	return (findWithTag(sets[index], tag)) != -1;
}


bool lineInvalNextIfShared(struct cache_t *cache, struct line_t *line, 
		uint32_t address)
{
	// invalidate lower caches if state is shared
	if (line->status == MESI_SHARED)
	{
		// shared line. Invalidate other caches
		if (VERBOSITY > 1)
			printf("Invalidating L2 line\n\t\t");
		//L2Invalidate(cache->next, address);
		return true;
	}
	return false;
}



void L1Reset(struct cache_t *cache)
{
	int i, j;
	uint32_t setsize, linesize;
	struct set_t *set;
	setsize = cache->setsize;
	linesize = cache->set[0]->linesize;
	for (i=0; i<setsize; i++)
	{
		set = cache->set[i];
		for (j=0; j<linesize; j++)
			set->line[j]->status = MESI_INVALID;
	}
	return;
}


void L1WriteLine(struct cache_t *cache, uint32_t address, uint32_t *data,
		enum Cache_Write_Type writeType)
{
	// write line with tag at index in set
	// evict and writeback if necessary

	uint32_t tag = getL1Tag(address),
		 index = getL1Index(address),
		 offset = getL1Offset(address);
	struct set_t *set = cache->set[index];
	int line = findWithTag(set, tag);

	if (line == -1)
	{	// Cache Miss
		// write new line; evict if necessary
		line = findLRU(set);
		if (set->lines[line]->status == MESI_MODIFIED)
		{
			// dirty line. Write to next level of cache
			if (VERBOSITY > 1)
				printf("Dirty line at %i. Write to L2 and evict from L1\n\t", line);
			L2WriteLine(L2, address, set->lines[line]->data, WRITE_ALLOCATE);
			set->line[line]->status = MESI_INVALID;
		}
		else if (set->line[line]->status != MESI_INVALID)
		{
			if (VERBOSITY > 1)
				printf("Evicting line %i.\n\t", line);
			set->line[line]->status = MESI_INVALID;
			free(set->line[line]->data);
		}
	}

	// TODO: should we invalidate or write to lower-level caches?
	lineInvalNextIfShared(cache, set->line[line], address);
	set->line[line]->data = data;
	if (writeType == WRITE_ALLOCATE)
	{
		if (L2CheckAddress(L2, address))
			set->line[line]->status = MESI_SHARED;
		else
			set->line[line]->status = MESI_EXCLUSIVE;
	}
	else
		set->line[line]->status = MESI_MODIFIED;
	set->line[line]->tag = tag;
	touchLRU(set, line);
	if (VERBOSITY > 1)
		printf("storing new value in line %i\n\t", line);

	return;
}


// L1 Cache Read/Write Handlers

uint32_t *L1Read(struct cache_t *cache, uint32_t address)
{
	// Read from the L1 cache

	uint32_t tag = getL1Tag(address),
		 index = getL1Index(address);
	uint8_t *lineValue;
	int line;
	struct set_t *set;
	bool local_hit;

	if (VERBOSITY)
		printf("L1:\tRead\n\t");

	cache->c_reads++;
	if (setCheckTagExists(cache->set, index, tag))
	{
		// Cache Hit
		local_hit = true;
		set = cache->set[index];
		line = findWithTag(set, tag);
		if (line == -1)
		{
			printf("error: should have a hit!\n");
			exit(EXIT_FAILURE);
		}
		touchLRU(set, line);
		lineValue = set->line[line]->data;
	}
	else
	{
		// Cache Miss
		local_hit = false;
		//lineValue = L2Read(L2, address);
		cache->ln_ops.read(address, 64, lineValue);
		L1WriteLine(cache, address, lineValue, WRITE_ALLOCATE);
	}
	cache->c_hits += local_hit;
	cache->c_misses += !local_hit;
	if (VERBOSITY)
	{
		if (!local_hit)
			printf("[R]:MISS\t");
		else
			printf("[R]:HIT\t\t");
		L1PrintLine(cache, address);
	}

	return lineValue;
}



void L1Write(struct cache_t *cache, uint32_t address, uint32_t data)
{
	// Write to L1 cache

	uint32_t tag = getL1Tag(address),
		 index = getL1Index(address),
		 offset = getL1Offset(address);
	uint8_t *lineValue;
	struct set_t *set;
	int line;
	bool local_hit;

	if (VERBOSITY)
		printf("L1:\tWrite\n\t");

	if (setCheckTagExists(cache->set, index, tag))
	{
		// Cache Hit
		local_hit = true;
		set = cache->set[index];
		line = findWithTag(set, tag);
		lineValue = set->line[line]->data;
		//setWriteWord(cache, address, data, WRITE_MODIFY);
	}
	else
	{
		// Cache Miss
		local_hit = false;
		//lineValue = L2Read(L2, address);
		cache->ln_ops.read(address, 64, lineValue);
	}

	// update statistics
	cache->c_writes++;
	cache->c_hits += local_hit;
	cache->c_misses += !local_hit;

	// store data
	lineValue[offset] = data;
	L1WriteLine(cache, address, lineValue, WRITE_MODIFY);

	if (VERBOSITY)
	{
		// print some extra info
		if (!local_hit)
			printf("[W]:MISS\t");
		else
			printf("[W]:HIT\t\t");
		L1PrintLine(cache, address);
	}

	return;
}


void L1Invalidate(struct cache_t *cache, uint32_t address)
{
	// Invalidate anything at address in cache
	
	uint32_t tag = getL1Tag(address),
		 index = getL1Index(address);

	if (VERBOSITY > 1)
	{
		// print some extra info
		printf("Invalidate L1 cache at address %i\n", address);
	}

	invalidateLRUWay(cache->set[index], findWithTag(cache->set[index], tag));

	return;
}


void L1PrintLine(struct cache_t *cache, uint32_t address)
{
	// print line in cache
	
	uint32_t tag = getL1Tag(address),
		 index = getL1Index(address),
		 offset = getL1Offset(address);
	struct set_t *set = cache->set[index];
	int line = findWithTag(set, tag);
	const char mesistate[] = "MESI";
	int mesi = set->line[line]->status;

	if (line == -1)
	{
		printf("Line not present with address 0x%x\n", address);
		printf("0x%x\n", (unsigned int)set->line[3]->data);
		return;
	}

	printf("0x%.8x:\t[%c]\t", address, mesistate[mesi]);
	printf("0x%.3x\t", tag);
	printf("%i\t", index);
	printf("%i\t", line);
	printf("%x\t", offset);
	printf("0x%x\n", (unsigned int)set->line[line]->data[offset]);
	printf("\n");

	return;
}


void L1PrintCache(struct cache_t *cache)
{
	// print all valid lines in the cache

	int i, j, k;
	struct set_t **sets = cache->set;
	struct line_t **lines;
	bool wrote_index = false;
	const char mesistate[] = "MESI";

	printf("\nMemory Map:\n");
	printf("MESI\tTAG\t");
	for (i=0; i<LINESIZE; i++)
		printf("    %x    ", i);
	printf("\n");

	for (i=0; i<cache->setsize; i++)
	{
		lines = sets[i]->line;
		for (j=0; j<sets[i]->linesize; j++)
		{
			if (lines[j]->status != MESI_INVALID)
			{
				if (!wrote_index)
				{
					printf("index %i:\n", i);
					wrote_index = true;
				}
				printf("[%c]\t0x%.3x\t", 
						mesistate[lines[j]->status], 
						lines[j]->tag);
				for (k=0; k<LINESIZE; k++)
					printf("%.8x ", lines[j]->data[k]);
				printf("\n");
			}
		}
		wrote_index = false;
	}
	return;
}

bool L1CheckAddress(struct cache_t *cache, uint32_t address)
{
	uint32_t tag = getL1Tag(address),
		 index = getL1Index(address);
	return setCheckTagExists(cache->set, index, tag);
}

