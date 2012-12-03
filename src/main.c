/* ECE485: Final Project
 * Authors:
 * Matthew Branstetter
 * Vernon Jones
 * Yusuf Qedan
 * Tyler Tricker
 *
 * Date: 19 Nov 2012
 */

/* External Modules Imports */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include "cache.h"

/* Global Constants */
static const unsigned long MAX_LINE = 1024;

enum COMMANDS
{
    COMMAND_READ,
    COMMAND_WRITE,
    COMMAND_IFETCH,
    COMMAND_INVALIDATE,
    COMMAND_RESET = 8,
    COMMAND_PRINT = 9,
    COMMAND_END
};


/* Forward Declarations*/
void static execute(struct cache_t* instruction,
		    struct cache_t* data, 
		    int op, 
		    uint32_t address);

/* Implementations */
int main(int argc, char** argv)
{
    FILE*    t_fileh;
    int      matched;
    int      op;
    uint32_t address; 
    char line[MAX_LINE+1];

    struct LN_ops_t L2_hooks;
    L2_hooks.read = NULL;
    L2_hooks.write = NULL;
    L2_hooks.modified = NULL;
    L2_hooks.evicted  = NULL;
    L2_hooks.object   = NULL;

    //caches
    struct cache_t *L1_instruction = cache_new(2, 16*1024, 64, L2_hooks);
    struct cache_t *L1_data        = cache_new(4, 16*1024, 64, L2_hooks);

    if(argc < 2)
    {
	printf("Usage: %s {tracefiles}\n", argv[0]);
	return 0;
    }


    for(int i = 1; i < argc; i++) //For every tracefile passed to the input
    {
	t_fileh = fopen(argv[i], "r");
	if(!t_fileh) //
	{
	    snprintf(line, MAX_LINE, "Skipping Entry %s", argv[i]);
	    perror(line);
	    continue;
	}
	printf("\n************ Running file: %s ************\n\n", argv[i]);
	/* Setup Cache */
	cache_reset(L1_instruction);
	cache_reset(L1_data);

	while(fgets(line, MAX_LINE, t_fileh)) //get line until end of file
	{
	    matched = sscanf(line, "%d%*[ \t]%x", &op, &address);
	    if (matched < 2) //TODO allow certain opcodes to not have an address
	    {
		if(matched && ((op == 8) || (op == 9)))
		{
		    // Valid command
		}
		else
		{
		    fprintf(stderr, "Parsing Warning - invalid line: %s", line);
		    continue;
		}
	    }
	    execute(L1_instruction, L1_data, op, address);
	}
	//Print out simulation Cache Results
	//hit rate = hits/(hits+misses) %
	float hitrate = (float) L1_data->stats.hits / 
	    ((float) L1_data->stats.hits + (float) L1_data->stats.misses) * 100.0f;

	printf("\nTrace Summary:\n");
	printf("\nData Cache\n");
	printf("Reads:%6zu      Writes:%6zu      Hits:%6zu      Misses:%6zu      HitRate:%4g%%\n",
	       L1_data->stats.reads,
	       L1_data->stats.writes,
	       L1_data->stats.hits,
	       L1_data->stats.misses,
	       hitrate);

	hitrate = (float) L1_instruction->stats.hits / 
	    ((float) L1_instruction->stats.hits + (float) L1_instruction->stats.misses) * 100.0f;

	printf("\nIntruction Cache\n");
	printf("Reads:%6zu      Writes:%6zu      Hits:%6zu      Misses:%6zu      HitRate:%4g%%\n",
	       L1_instruction->stats.reads,
	       L1_instruction->stats.writes,
	       L1_instruction->stats.hits,
	       L1_instruction->stats.misses,
	       hitrate);

	//Clean up resources
	if (fclose(t_fileh) == EOF)
	    perror("Failed to release file handle, continuing anyway");
	t_fileh = 0;
    }

    cache_free(L1_instruction);
    cache_free(L1_data);
    return 0;
}

/****************** Internal Implementation **********************/
void static execute(struct cache_t* instruction, 
		    struct cache_t* data, 
		    int op, 
		    uint32_t address)
{
    // Data Stub for read and write functions
    uint8_t cpu_port = 0xFF;
    
    switch(op)
    {
    case COMMAND_READ:		// (0)
	cache_readop(data, address, 1, &cpu_port);
	break;
    case COMMAND_WRITE:		// (1)
	cache_writeop(data, address, 1, &cpu_port);
	break;
    case COMMAND_IFETCH:	// (2)
	cache_readop(instruction, address, 1, &cpu_port);
	break;
    case COMMAND_INVALIDATE:	// (3)
	cache_invalidate(data, address);
	break;
    case COMMAND_RESET:		// (8)
	cache_reset(instruction);
	cache_reset(data);
	break;
    case COMMAND_PRINT:		// (9)
	printf("Printing Instruction Cache\n");
	cache_print(instruction);
	printf("Printing Data Cache\n");
	cache_print(data);
	break;
    default:
	fprintf(stderr, "Error: Invalid operation %d  \n", op);
    }
}
