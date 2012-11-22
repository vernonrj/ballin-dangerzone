#include <stdbool.h>
#include <stdlib.h>
#include <math.h>   //for calculating bit vector sizes


//TEMP INCLUDES
#include <stdio.h>

//Local includes
#include "cache.h"

uint16_t cache_lru_get_oldest_line(struct cache_t* cacheobj, uint32_t address)
{
	union bitfield_u bitfield;
	uint32_t tag, index, offset;
	struct set_t *set;

	bitfield.address = address;
	tag = bitfield.field.tag;
	index = bitfield.field.index;

	set = cacheobj->set[index];
	for (int i=0; i<cacheobj->linesize; i++)
	{
		if (set->line[i]->lru == cacheobj->linesize-1)
			return i;
	}
	return -1;
}

void cache_lru_update_way(struct cache_t* cacheobj, uint32_t address,
		uint16_t way)
{
	union bitfield_u bitfield = {address};
	uint32_t index = bitfield.field.index;
	struct set_t* set = cacheobj->set[index];
	uint16_t pivot = set->line[way]->lru;

	for (int i=0; i<cacheobj->linesize; i++)
	{
		if (i == way)
			set->line[way]->lru = 0;
		else if (set->line[way]->lru < pivot)
			++set->line[way]->lru;
	}
	return;
}



bool cache_address_resident(struct cache_t* cacheobj, uint32_t address,
		uint16_t *way)
{
	union bitfield_u bitfield;
	uint32_t tag, index, offset;
	struct set_t *set;

	bitfield.address = address;
	tag = bitfield.field.tag;
	index = bitfield.field.index;
	offset = bitfield.field.offset;

	set = cacheobj->set[index];
	for (int i=0; i<cacheobj->linesize; i++)
	{
		if (set->line[i]->tag == tag)
		{
			*way = i;
			return true;
		}
	}
	return false;
}


/* Allocates a new cache object with the following parameters
 *
 */
struct cache_t* cache_new(size_t associativity, 
			size_t index_size,
			size_t line_size, 
			struct LN_ops_t interface)
{
    struct cache_t* new_cache = malloc(sizeof(struct cache_t) + 
				     index_size*sizeof(struct set_t*));
    if(!new_cache) return NULL; //watch for malloc errors
    //Set Parameter structure
    new_cache->params.associativity = associativity;
    new_cache->params.line_size = line_size;
    new_cache->params.index_size = index_size;

    //Allocate Sets
    for(int i = 0; i < index_size; ++i)
    {
	new_cache->set[i] = malloc(sizeof(struct set_t) +
				   line_size*sizeof(struct line_t*));
	//Allocate Lines + data
	for(int j = 0; j < associativity; ++j)
	    new_cache->set[i]->line[j] = malloc(sizeof(struct line_t) + 
						line_size*sizeof(uint8_t));
    }
    cache_reset(new_cache);
    return new_cache;
    //TODO add clean up code if malloc fails in one of the loops
}

/* deallocate the memory used by the cache obj */
void cache_free(struct cache_t* cacheobj)
{
    //Allow NULL free
    if(!cacheobj) return;

    for(int i = 0; i < cacheobj->params.index_size; ++i)
    {
	for(int j = 0; j < cacheobj->params.associativity; ++j)
	{
	    free(cacheobj->set[i]->line[j]); //free everyline in every set
	}
	free(cacheobj->set[i]); //free everyset
    }
    free(cacheobj);
}

/* Clear Status bits and setup check LRU */
void cache_reset(struct cache_t* cacheobj)
{
    struct status_t empty_status;
    empty_status.valid = false;
    empty_status.dirty = false;

    cacheobj->params.lru_bits = (uint16_t)(
	log(cacheobj->params.associativity) / log(2));

    for(int i = 0; i < cacheobj->params.index_size; ++i)
    {
	for(int j = 0; j < cacheobj->params.associativity; ++j)
	{
	    cacheobj->set[i]->line[j]->status = empty_status;
	    cacheobj->set[i]->line[j]->lru = j; //Prime LRU values
	}
    }

}

/* Do a read operation */
void cache_readop(struct cache_t* cacheobj, uint32_t address)
{
	union bitfield_u bitfield = {address};
	uint32_t index = bitfield.field.index;
	uint16_t way;
	uint8_t* data;
	++cacheobj->stats.reads;
	if (cache_address_resident(cacheobj, address, &way))
	{
		// Cache Hit
		++cacheobj->stats.hits;
	}
	else
	{
		// Cache Miss
		++cacheobj->stats.misses;
		way = cache_lru_get_oldest_line(cacheobj, address);
		cacheobj->ln_ops.read(address, 16, data);
		//cacheobj->set[index]->line[way] = data;
	}
	cache_lru_update_way(cacheobj, address, way);
	return;
}


