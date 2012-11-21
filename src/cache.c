#include <stdlib.h> //for size_t
#include <math.h>   //for calculating bit vector sizes

//TEMP INCLUDES
#include <stdio.h>

//Local includes
#include "cache.h"

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

    cacheobj->params.lru_bits = (uint16_t)(
	log(cacheobj->params.associativity) / log(2));

}

