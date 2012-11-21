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

//include "instruction_cache_model.h"
//include "data_cache_model.h"

//TODO come up with error.log

/* Global Constants */
static const unsigned long MAX_LINE = 1024;

/* Forward Declarations */
void static execute(int op, uint32_t address);

/* Implementations */
int main(int argc, char** argv)
{
    FILE*    t_fileh;
    int      matched;
    int      op;
    uint32_t address; 
    char line[MAX_LINE+1];
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
	while(fgets(line, MAX_LINE, t_fileh)) //get line until end of file
	{
	    matched = sscanf(line, "%d%*[ \t]%x", &op, &address);
	    if (matched < 2)
	    {
		fprintf(stderr, "Parsing Failed\n");
		continue;
	    }
	    execute(op, address);
	}
	//Clean up resources
	if (fclose(t_fileh) == EOF)
	    perror("Failed to release file handle, continuing anyway");
	t_fileh = 0;
    }
    return 0;
}


void static execute(int op, uint32_t address)
{
    //DELETE this line - For verbose checking
    printf("Execute %d %x\n", op, address);
    switch(op)
    {
    case 0:
	break;
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	break;
    default:
	printf("Error: Invalid operation\n");
    }
}
