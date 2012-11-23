#include <stdbool.h>
#include <stdlib.h>
#include <math.h>   //for calculating bit vector sizes


//TEMP INCLUDES
#include <stdio.h>

//Local includes
#include "cache.h"


/* Forward Declaration */
static void lru_update_set(const struct cache_params_t* params, 
			   struct set_t* set,
			   uint16_t way);

static struct line_t* cache_access(struct cache_t* cacheobj, uint32_t address);

  // Address access functions
static uint32_t address_get_tag(const struct cache_params_t* params,
				uint32_t address);
static uint32_t address_get_index(const struct cache_params_t* params,
				  uint32_t address);
static uint32_t address_get_byte_offset(const struct cache_params_t* params,
					uint32_t address);

// for no op callbacks
op_status_t null_fop(uint32_t address, size_t length, uint8_t* data){ return 0;}; 


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

    //Set LN_ops
    new_cache->ln_ops.read = interface.read   ? interface.read  : null_fop;
    new_cache->ln_ops.write = interface.write ? interface.write : null_fop;
    new_cache->ln_ops.modified = 
	interface.modified ? interface.modified : null_fop;

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

/******************* CACHE IMPLEMENTATION FUNCTIONS ************************/

/* Do a read operation */
uint8_t* cache_readop(struct cache_t* cacheobj, uint32_t address)
{
    uint32_t byte_offset = address_get_byte_offset(&cacheobj->params, address); 

    ++cacheobj->stats.reads;
    struct line_t* line = cache_access(cacheobj, address);
    return &line->data[byte_offset];

}


uint8_t* cache_writeop(struct cache_t* cacheobj, uint32_t address)
{
    uint32_t byte_offset = address_get_byte_offset(&cacheobj->params, address); 

    ++cacheobj->stats.writes;
    struct line_t* line = cache_access(cacheobj, address);
    return &line->data[byte_offset];

}

void cache_invalidate(struct cache_t* cacheobj, uint32_t address)
{
    //TODO implement
}

/******************** Internal Module Functions *******************/

/* processes the internal cache structures and returns a line to write data
*  or read from it.*/
static struct line_t* cache_access(struct cache_t* cacheobj, uint32_t address)
{
    const struct cache_params_t* params = &cacheobj->params;
    uint32_t index = address_get_index(params, address);
    uint32_t tag   = address_get_tag(params, address);
    struct line_t* line;
    int lru     = 0;
    int invalid = -1;

    // Search in the set for valid invalid entries etc..
    for(int i = 0; i < params->associativity; ++i)
    {
	line = cacheobj->set[index]->line[i];

	if(line->status.valid) //find valid match
	{
	    if(tag == line->tag) //Match tags
	    {
		// Hit
		++cacheobj->stats.hits;
		lru_update_set(&cacheobj->params, 
			       cacheobj->set[index], 
			       i);
		return line;
	    }
	    else //look for LRU
	    {
    	      	lru = line->lru > cacheobj->set[index]->line[lru]->lru ? 
		    i : lru;
	    }
	}
	else 
	    invalid = i;
    }
    ++cacheobj->stats.misses;

    // Cache Miss 
    if(-1 < invalid) // if there is an invalid use it
    {
	line = cacheobj->set[index]->line[invalid];
	line->status.valid = true;
	lru_update_set(&cacheobj->params, cacheobj->set[index], invalid);
    }
    else            // if not use the lru
    {
	// Evict LRU by writeback if dirty
	if(line->status.dirty)
	    cacheobj->ln_ops.write(address, 
				   cacheobj->params.line_size, 
				   line->data);
	cacheobj->ln_ops.read(address, 
			      cacheobj->params.line_size, 
			      line->data);
	line = cacheobj->set[index]->line[lru];
	lru_update_set(&cacheobj->params, cacheobj->set[index], lru);
    }
    return line;
}

/* Updates the sets LRU values depending on the way just accessed */
static void lru_update_set(const struct cache_params_t* params, 
			   struct set_t* set,
			   uint16_t way)
{
    uint16_t pivot = set->line[way]->lru;

    for (int i = 0; i < params->associativity; ++i)
    {
	if(i == way)
	    set->line[way]->lru = 0;
	else if (set->line[way]->lru < pivot)
	    ++set->line[way]->lru;
    }
}

/*********************** Address access functions ***************************/

// returns only the tag bits in an address
static uint32_t address_get_tag(const struct cache_params_t* params,
				uint32_t address)
{
    uint32_t ntag_bit_size = log2(params->line_size) + log2(params->index_size);
    uint32_t tag = address >> ntag_bit_size; //mask out lower bits
    return tag;
}

// returns only the set index given an address
static uint32_t address_get_index(const struct cache_params_t* params,
				      uint32_t address)
{
    uint32_t index = address >> (uint32_t)log2(params->line_size);
    uint32_t field = ~0 << (uint32_t)log2(params->index_size);

    index = index & ~field;
    
    return index;
}

static uint32_t address_get_byte_offset(const struct cache_params_t* params,
					uint32_t address)
{
    size_t line_index = log2(params->line_size);
    uint32_t field = ~0; //Fill with 0xFF..
    line_index = (field >> (32 - line_index)) & address; //mask out upper bits
    return line_index;
}
