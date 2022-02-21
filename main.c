#include <string.h>
#include <stdio.h>
#include <unistd.h>


void process_args(int argc, char* argv[]) {
void print_usage();

void main(int argc, char *argv[]) {

    // Process args
    printf("arc: %d", argc);
    

}

void print_usage() {
    printf("Usage: -alg [FIFO|SJF|PR|RR] [-quantum [integer(ms)]] -input [file name]\n");
}

int valid_alg(char * alg) {
    if (!strcmp("FIFO", alg) return 1;
    if (!strcmp("SJF", alg) return 1;
    if (!strcmp("PR", alg) return 1;
    if (!strcmp("RR", alg) return 1;
    return 0;
}

void process_args(int argc, char* argv[]) {
    // need to at least specify an algorithm
    if (argc < 3) {
        print_usage();
    }
    
    // Checking first argument
    if (strcmp(argv[1], "-arg")) {
        print_usage();
    }
    if (!valid_alg(argv[2])) {
        print_usage();
        exit(-1);
    } 

    // Checking for quantum/input file
    if (argc == 4) {
        // Missing an argument value
        print_usage();
    }
    // argc >= 5
    
}


