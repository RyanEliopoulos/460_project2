/*
 *
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

void main(int argc, char *argv[]) {
    printf("argc: %d\n", argc);

    unsigned int t_slice;
    if (!valid_args(argc, argv, &t_slice)) {
        print_usage();
        exit(-1);
    }
    // Argument format valid. Input file error checked in relevant thread.
    printf("Arguments check out. Creating semaphores\n");
    sem_t *ready_sem = sem_open("ready_sem", O_CREAT, O_RDWR, 1);
    printf("ready sem: %s\n", strerror(errno));
    sem_t *io_sem = sem_open("io_sem", O_CREAT, O_RDWR, 1);
    printf("io sem: %s\n", strerror(errno));
    // Creating fileread thread
    pthread_t fileread_thread;
    //int ret = pthread_create(&fileread_thread, 
}

void print_usage() {
    printf("Usage: -alg [FIFO|SJF|PR|RR] [-quantum [integer(ms)]] -input [file name]\n");
}


