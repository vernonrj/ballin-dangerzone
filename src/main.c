/* ECE411: Final Project
 * Authors:
 * 
 * Date: 19 Nov 2012
 */

/* External Modules Imports */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include "cache.h"

//TODO add error.log

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


/* Forward Declarations - TODO change to include cache struct */
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

    //caches
    struct cache_t *L1_instruction = cache_new(2, 1024, 64, L2_hooks);
    struct cache_t *L1_data        = cache_new(4, 1024, 64, L2_hooks);

    if(argc < 2)
    {
	printf("Usage: %s {tracefiles}\n", argv[0]);
	return 0;
    }

    /* Setup Cache */
    

    /* Run Test Files - TODO: Should probably clear cache between files */
    for(int i = 1; i < argc; i++) //For every tracefile passed to the input
    {
	t_fileh = fopen(argv[i], "r");
	if(!t_fileh) //
	{
	    snprintf(line, MAX_LINE, "Skipping Entry %s", argv[i]);
	    perror(line);
	    continue;
	}
	while(fgets(line, MAX_LINE, t_fileh)) //get line until end of file
	{
	    matched = sscanf(line, "%d%*[ \t]%x", &op, &address);
	    if (matched < 2)
	    {
		fprintf(stderr, "Parsing Failed\n");
		continue;
	    }
	    execute(L1_instruction, L1_data, op, address);
	}
	//Clean up resources
	if (fclose(t_fileh) == EOF)
	    perror("Failed to release file handle, continuing anyway");
	t_fileh = 0;
    }

    cache_free(L1_instruction);
    cache_free(L1_data);
    return 0;
}


void static execute(struct cache_t* instruction, 
		    struct cache_t* data, 
		    int op, 
		    uint32_t address)
{
    //DELETE this line - For verbose checking
    printf("Execute %d %x\n", op, address);
    switch(op)
    {
    case COMMAND_READ:		// (0)
//	cache_readop(data, address);
	break;
    case COMMAND_WRITE:		// (1)
//	cache_writeop(data, address);
	break;
    case COMMAND_IFETCH:	// (2)
//	cache_readop(instruction, address);
	break;
    case COMMAND_INVALIDATE:	// (3)
	cache_invalidate(data, address);
	break;
    case COMMAND_RESET:		// (8)
	cache_reset(data);
	break;
    case COMMAND_PRINT:		// (9)
	break;
    default:
	printf("Error: Invalid operation\n");
    }
}
