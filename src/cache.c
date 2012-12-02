#include <stdbool.h>
#include <stdlib.h>
#include <math.h>   //for calculating bit vector sizes
#include <stdio.h>

//Local includes
#include "cache.h"


/* Forward Declarations */
static struct line_t* cache_access(struct cache_t* cacheobj, uint32_t address);
static void lru_update_set(const struct cache_params_t* params, 
			   struct set_t* set,
			   uint16_t way);
static void print_set(const struct cache_params_t* params,
		      struct set_t* set);

/* Address access functions */
static uint32_t address_get_tag(const struct cache_params_t* params,
				uint32_t address);
static uint32_t address_get_index(const struct cache_params_t* params,
				  uint32_t address);
static uint32_t address_get_byte_offset(const struct cache_params_t* params,
					uint32_t address);

// for no op callbacks
op_status_t null_fop(void* object, 
		     uint32_t address, 
		     size_t length, 
		     uint8_t* data)
{ return 0;}

op_status_t null_modified(void* object,
			  uint32_t address)
{return 0;}

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
	interface.modified ? interface.modified : null_modified;
    new_cache->ln_ops.evicted = 
	interface.evicted ? interface.evicted : null_modified;

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

    // set lru_bits size to determine how many bits are required
    cacheobj->params.lru_bits = (uint16_t)(
	log(cacheobj->params.associativity) / log(2));

    // Reset statistics
    cacheobj->stats.hits   = 0;
    cacheobj->stats.misses = 0;
    cacheobj->stats.reads  = 0;
    cacheobj->stats.writes = 0;

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
op_status_t cache_readop(struct cache_t* cacheobj, 
		      uint32_t address,
		      size_t length,
		      uint8_t* data)
{
    uint32_t byte_offset = address_get_byte_offset(&cacheobj->params, address); 

    ++cacheobj->stats.reads;
    struct line_t* line = cache_access(cacheobj, address);

    int i;
    for(i = 0; (i < length)&&(i < cacheobj->params.line_size); ++i)
    {
	data[i] = line->data[byte_offset+i];
    }

    return i; // returns number of characters written to buffer
}


op_status_t cache_writeop(struct cache_t* cacheobj, 
			  uint32_t address,
			  size_t length,
			  uint8_t* data)
{
    uint32_t byte_offset = address_get_byte_offset(&cacheobj->params, address); 

    ++cacheobj->stats.writes;
    struct line_t* line = cache_access(cacheobj, address);

    if(!line->status.dirty)
    {
	cacheobj->ln_ops.modified(cacheobj->ln_ops.object, address);
    }
    line->status.dirty = true;

    int i;
    for(i = 0; (i < length)&&(i < cacheobj->params.line_size); ++i)
    {
	line->data[byte_offset+i] = data[i];
    }

    return i; //number of characters read

}

op_status_t cache_invalidate(struct cache_t* cacheobj, uint32_t address)
{
    uint32_t index    = address_get_index(&cacheobj->params, address);
    uint32_t tag      = address_get_tag(&cacheobj->params, address);
    struct set_t* set = cacheobj->set[index];
    
    for(int i = 0; i < cacheobj->params.associativity; ++i)
    {
	if(tag == set->line[i]->tag) // match line
	{
	    struct line_t* line = set->line[i];
	    if(line->status.valid && line->status.dirty) // is line dirty?
		cacheobj->ln_ops.write(cacheobj->ln_ops.object, 
				       address, 
				       cacheobj->params.line_size, 
				       line->data); // write back

	    line->status.valid = false; // invalidate line
	    line->status.dirty = false;
	}
	// just ignore request if not in cache
    }
    return 0;
}

void cache_print(const struct cache_t* cacheobj)
{
    printf("Cache Parameters: \n");
    printf("Sets:%6zu Line Length: %6zu Associativity: %6zu \n\n",
	   cacheobj->params.index_size,
	   cacheobj->params.line_size,
	   cacheobj->params.associativity);
    printf("%11c",' ');
    for(int i = 0; i < cacheobj->params.associativity; ++i)
    {
	printf("VD  LRU      TAG%1c",' ');
    }
    printf("\n");

    for(int i = 0; i < cacheobj->params.index_size; ++i)
    {
	// Check if the line is worth printing
	bool valid_set = false;
	struct set_t* set = cacheobj->set[i];

	for(int j = 0; j < cacheobj->params.associativity; ++j)
	{
	    valid_set = valid_set | set->line[j]->status.valid;
	}
	
	if(valid_set)
	{
	    printf("set:  %4d ", i);
	    print_set(&cacheobj->params, cacheobj->set[i]);
	}
    }
    //Print Statistics
    //hit rate = hits/(hits+misses) %
    float hitrate = (float) cacheobj->stats.hits / 
	((float) cacheobj->stats.hits + (float) cacheobj->stats.misses) * 100.0f;

    printf("\nReads:%6zu      Writes:%6zu      Hits:%6zu      Misses:%6zu      HitRate:%4g%%\n",
	   cacheobj->stats.reads,
	   cacheobj->stats.writes,
	   cacheobj->stats.hits,
	   cacheobj->stats.misses,
	   hitrate);

}


/******************** Internal Module Functions *******************/

/* processes the internal cache structures and returns a line to write data
*  or read from it.*/
static struct line_t* cache_access(struct cache_t* cacheobj, uint32_t address)
{
    const struct cache_params_t* params = &cacheobj->params;
    uint32_t index = address_get_index(params, address);
    uint32_t tag   = address_get_tag(params, address);
    uint32_t byte_offset = address_get_byte_offset(params, address);
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
	lru_update_set(&cacheobj->params, cacheobj->set[index], invalid);
    }
    else            // if not use the lru
    {
	line = cacheobj->set[index]->line[lru];
	// Evict LRU by writeback if dirty
	if(line->status.dirty)
	{
	    cacheobj->ln_ops.write(cacheobj->ln_ops.object,
				   address ^ byte_offset, 
				   cacheobj->params.line_size, 
				   line->data);
	    line->status.dirty = false;
	}
	else
	{
	    cacheobj->ln_ops.evicted(cacheobj->ln_ops.object,
				   address ^ byte_offset);
	}
	// continue request
	cacheobj->ln_ops.read(cacheobj->ln_ops.object,
			      address ^ byte_offset, 
			      cacheobj->params.line_size, 
			      line->data);
	lru_update_set(&cacheobj->params, cacheobj->set[index], lru);
    }

    line->status.valid = true;
    line->tag = tag;
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
	else if (set->line[i]->lru < pivot)
	    ++set->line[i]->lru;
    }
}

static void print_set(const struct cache_params_t* params,
		      struct set_t* set)
{

    // Output Format
    // VD TAG '' VD TAG '' VD TAG '' VD TAG
    for(int i = 0; i < params->associativity; ++i)
    {
	struct line_t* line = set->line[i];
	if(line->status.valid)
	{
	    printf("%d%d %4d %8x ", 
		   line->status.valid, 
		   line->status.dirty,
		   line->lru,
		   line->tag);
	}
	else
	{
	    printf("%17c", ' '); //Leave empty
	}
    }
    printf("\n");
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

// returns the byte offset of an address
static uint32_t address_get_byte_offset(const struct cache_params_t* params,
					uint32_t address)
{
    size_t line_index = log2(params->line_size);
    uint32_t field = ~0; //Fill with 0xFF..
    line_index = (field >> (32 - line_index)) & address; //mask out upper bits
    return line_index;
}
