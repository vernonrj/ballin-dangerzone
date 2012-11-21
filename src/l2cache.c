// L2 cache functions
#include "cache.h"
#define INDEXSIZE 14	// 14-bit indexes
#define OFFSETSIZE 4	// 4-bit offset
#define TAGSIZE (32 - INDEXSIZE - OFFSETSIZE)

// Stubs for L2 cache
void L2Reset(struct cache_t *cache)
{
	L1Reset(L1I);
	L1Reset(L1D);
	return;
}

void L2WriteLine(struct cache_t *cache, uint32_t address, size_t length, uint8_t *data)
{
	// write a line to the L2 cache
	// stub
	if (!data)
	{
		printf("data is null!\n");
	}
	free(data);
	return;
}

void L2Read(struct cache_t *cache, uint32_t address, size_t length, uint8_t *data)
{
	// read from L2 cache
	//printf("Read from L2\n");
	int i;
	uint8_t *line;
	uint32_t offset = getOffset(address, INDEXSIZE, OFFSETSIZE);
	uint32_t index = getIndex(address, INDEXSIZE, OFFSETSIZE);
	int data_size = cache->set[0]->line[0]->data_size;

	line = (uint8_t*)malloc(data_size*sizeof(uint8_t));
	for (i=0; i<data_size; i++)
	{
		//line[i] = i+(index << 4);
		line[i] = 0x0;
	}
	line[offset] = address;

	data = line;
	return;
}

void L2Write(struct cache_t *cache, uint32_t address, size_t length, uint8_t *data)
{
	// write to L2 cache
	//printf("Write to L2\n");

	return;
}

void L2Invalidate(struct cache_t *cache, uint32_t address)
{
	// Invalidate address in L2 cache
	//printf("Invalidate Line in L2\n");
	L1Invalidate(L1D, address);
	//L1Invalidate(L1I, address);
	return;
}

bool L2CheckAddress(struct cache_t *cache, uint32_t address)
{
	if (L2USESHARED)
	{
		return true;
	}
	else
	{
		return false;
	}
}
