/*
 * bacon
 *  Need a ready Q semaphore and an io_q semaphore
 *
 *
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "args.h"


void print_usage();
void *threading_test();

void main(int argc, char *argv[]) {
    printf("argc: %d\n", argc);

    unsigned int t_slice = 0;
    if (!valid_args(argc, argv, &t_slice)) {
        print_usage();
        exit(-1);
    }
    printf("time slice: %u\n", t_slice);
    // Argument format valid. Input file error checked in relevant thread.
    printf("Arguments check out. Creating semaphores\n");
    sem_t ready_sem;
    sem_t io_sem;

    if (sem_init(&ready_sem, 0, 1) == -1) {
        printf("ready sem: %s\n", strerror(errno));
        errno = 0;
    }
    if (sem_init(&io_sem, 0, 1) == -1) {
        printf("io sem: %s\n", strerror(errno));
    }

    // Creating fileread thread
    pthread_t fileread_thread;
    int ret = pthread_create(&fileread_thread, NULL, (*threading_test), NULL);
    printf("ret: %d\n", ret);
    pthread_join(fileread_thread, NULL);
}   

void print_usage() {
    printf("Usage: -alg [FIFO|SJF|PR|RR] [-quantum [integer(ms)]] -input [file name]\n");
}


void *threading_test() {

    for (int i = 0; i < 10; i++) {
        printf("in other thread %d\n", i);
    }
     
}

