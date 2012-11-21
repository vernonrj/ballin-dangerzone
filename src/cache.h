#ifndef __CACHE_H__
#define __CACHE_H__

#include <stdint.h>

/*************************Data Structures************************************/

/* status bits structure for each line
 */
struct status_t
{
    unsigned valid:1;
    unsigned dirty:1;
};

/* Line Structure for every set
 * LRU space complexity N*log2(N)
 */
struct line_t
{
    uint16_t lru;		// LRU bits: max assoc limited to 2^16
    uint32_t tag;               // cache tag
    struct status_t status;            // dirty and valid bits
    uint8_t  data[];	        // line data
};

/* Set structure: 
 */
struct set_t
{
    uint8_t stub;                       // stub for internal set data
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

struct cache_params_t
{
    size_t associativity;
    size_t line_size;
    size_t index_size;
    uint16_t lru_bits;           // number of bits used in LRU - Calculated
};

struct cache_statistics_t
{
    size_t hits;
    size_t misses;
    size_t reads;
    size_t writes;
};

/* Main cache data structure */
struct cache_t
{
    // cache data structure
    int setsize;     // number of sets
    int linesize;    // number of lines per set
    size_t data_size;// number of bytes in data
    struct cache_statistics_t stats;
    struct cache_params_t     params; //size parameters of the cache
    struct LN_ops_t           ln_ops; //callbacks to the lower memory hierarchy
    struct set_t              *set[]; //
};

/***************** Cache API *************************************************/
/* line size     = number of bytes in a line (should be a power of 2 (1 << N))
 * index size    = number of sets  (should be a power of 2 (1 << N))
 * associativity = number of lines per set (should be a power of 2 (1 << N))
 */

struct cache_t* cache_new(size_t associativity, 
			size_t index_size,
			size_t line_size, 
			struct LN_ops_t interface);
void cache_free(struct cache_t* cacheobj);
void cache_reset(struct cache_t* cacheobj);

// input operations
void cache_readop(struct cache_t* cacheobj, uint32_t address);
void cache_writeop(struct cache_t* cacheobj, uint32_t address);
void cache_invalidate(struct cache_t* cacheobj, uint32_t address);

// debuging functions
void cache_print(const struct cache_t* cacheobj);

#endif //__CACHE_H__
