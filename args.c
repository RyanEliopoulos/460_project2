
#include "args.h"
#include <string.h>
#include <stdio.h>



int valid_alg(char *alg) {
    if (!strcmp("FIFO", alg)) return 1;
    if (!strcmp("SJF", alg)) return 1;
    if (!strcmp("PR", alg)) return 1;
    if (!strcmp("RR", alg)) return 1;
    return 0;
}

int valid_file(char *in_arg, char *file_name) {
    
}

int valid_args(int argc, char* argv[], unsigned int *t_slice) {
    /* Evaluates the command line arguments for validity.
     Does not attempt to open the input file 

     returns: 1 if args are valid, else 0 */

    if (argc != 3 && argc != 5 && argc != 7) {
        printf("Arg count fail\n");
        return 0;
    }
    // Checking first argument
    if (strcmp(argv[1], "-alg")) {
        printf("alg input fail\n");
        return 0;
    }
    if (!valid_alg(argv[2])) {
        return 0;
    }   
    // Checking for -input switch
    if (strcmp("-input", argv[argc - 2])) {
        printf("input switch fail\n");
        return 0;
    } 
    // Evaluating -quantum switch and arg
    if (argc == 7) {
        if (strcmp("-quantum", argv[3])) {
            return 0;
        } 
        // sscanf to get the integer here.
        int ret = sscanf(argv[4], "%d", t_slice);
        if (ret != 1) {
            return 0;
        }
        
    }
    return 1;
}

