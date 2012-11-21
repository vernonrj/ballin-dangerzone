#ifndef __CACHE_H__
#define __CACHE_H__

#include <stdint.h>

// Data Structures

/* status bits structure for each line
 */
struct status_t
{
<<<<<<< Updated upstream
	COMMAND_READ 		= 0,
	COMMAND_WRITE 		= 1,
	COMMAND_IFETCH 		= 2,		// read request from L1
	COMMAND_INVALIDATE 	= 3,	// comes from L2
	COMMAND_RESET 		= 8,	
	COMMAND_PRINT 		= 9
=======
    unsigned valid:1;
    unsigned dirty:1;
>>>>>>> Stashed changes
};

// Line Structure for every set
struct line_t
{
    uint32_t tag;               // cache tag
    status_t status;            // dirty and valid bits
    uint8_t  data[];	        // line data
};

// Set structure
struct set_t
{
<<<<<<< Updated upstream
    uint32_t *lru;			// LRU bits:
    //int lrusize;			// number of bits in LRU
=======
    uint8_t *lru;			// LRU bits:
    int lrusize;			// number of bits in LRU
>>>>>>> Stashed changes
					// Space complexity: N*log2(N)
    struct line_t *line[];		// lines in set
};

/* Lower Memory Level callback functons
 * read  - get a data line from the lower level
 * write - write back data to lower level
 * modified - notifies the lower level cache that this value
 * has changed from clean to dirty. Set these operators to NULL if they are
 * not used.
 */

typedef int op_status_t;
struct LN_ops_t
{
    op_status_t (*read)(uint32_t address, size_t length, uint8_t* data);
    op_status_t (*write)(uint32_t address, size_t length, uint8_t* data);
    op_status_t (*modified)(uint32_t address, size_t length, uint8_t* data);
};

struct cache_params
{
    size_t associativity;
    size_t line_size;
    size_t index_size;
};

struct cache_statistics_t

/* Main cache data structure */
struct cache_t
{
<<<<<<< Updated upstream
    // cache data structure
    int setsize;     // number of sets
    int linesize;    // number of lines per set
    size_t data_size;// number of bytes in data
    int c_reads,     // cache statistics
	c_writes,
	c_hits,
	c_misses;
    struct LN_ops_t ln_ops;     //callbacks to the lower memory hierarchy
    struct set_t *set[];        //
=======
    struct cache_statistics_t stats;
    struct cache_params_t     params; //size parameters of the cache
    struct LN_ops_t           ln_ops; //callbacks to the lower memory hierarchy
    struct set_t              *set[]; //
>>>>>>> Stashed changes
};

/***************** Cache API *************************************************/
/* line size     = number of bytes in a line
 * index size    = number of sets  
 * associativity = number of lines per set 
 */

struct cache* cache_new(size_t associativity, 
			size_t line_size, 
			size_t index_size,
			struct LN_ops_t interface);
void cache_free(struct cache* cacheobj);
void cache_reset(struct cache* cacheobj);

// input operations
void cache_readop(struct cache* cacheobj, uint32_t address);
void cache_writeop(struct cache* cacheobj, uint32_t address);
void cache_invalidate(struct cache* cacheobj, uint32_t address);

// debuging functions
void cache_print(const struct cache* cacheobj);

#endif //__CACHE_H__

