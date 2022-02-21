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



#define BUFFERSIZE 65536




struct PCB {

    int pid, pr;
    int numCPUBurst, numIOBurst;
    int *CPUBurst, *IOBurst;
    int cpuindex, ioindex;
    struct timespec ts_begin, ts_end;
    struct PCB *prev, *next;
};

// general
void print_usage();
void *fileread();  // file read thread
// list management
void printPCBs(struct PCB*);
void freePCBs(struct PCB*);
void makeAndAddPCB(char* info, int newPID);
void readyAddPCB(struct PCB*);


sem_t ready_sem;
sem_t io_sem;

struct PCB *ready_q = NULL;
struct PCB *io_q = NULL;

file_read_done = 0;  // updated to 1 upon completion

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

    if (sem_init(&ready_sem, 0, 1) == -1) {
        printf("ready sem: %s\n", strerror(errno));
        errno = 0;
    }
    if (sem_init(&io_sem, 0, 1) == -1) {
        printf("io sem: %s\n", strerror(errno));
    }

    // Creating fileread thread
    pthread_t fileread_thread;
    char *filepath = argv[argc - 1];
    int ret = pthread_create(&fileread_thread, NULL, (*fileread), (void *)filepath);
    printf("ret: %d\n", ret);
    // Creating CPU thread

    // Awaiting threads
    pthread_join(fileread_thread, NULL);

    printPCBs(ready_q);
}   

void print_usage() {
    printf("Usage: -alg [FIFO|SJF|PR|RR] [-quantum [integer(ms)]] -input [file name]\n");
}


void *fileread(char *filepath) {
    // For the file read thread. 
    // Error checks the filepath

    // Opening the file and reading into q
    FILE *fp = fopen(filepath, "r");
    printf("file: %s\n", filepath);
    if (fp == NULL) {
        printf("couldn't open file\n");
        printf("%s\n", strerror(errno));
        exit(-1);
    } 
    // Preparing to read data
    char readBuffer[BUFFERSIZE] = "";  
    char info[BUFFERSIZE] = ""; 
    char *token;
    int currentPID = 0; 
    while(fgets(readBuffer, BUFFERSIZE, fp) != NULL) {
        strcpy(info, readBuffer);
        token = strtok(readBuffer, " ");
        if(!strcmp(token, "proc")) {
            // New PCB for the ready queue
            printf("Creating new PCB here\n");
            makeAndAddPCB(info, currentPID);
            currentPID++;
        }
        else if (!strcmp(token, "sleep")) {
            printf("fileread is sleeping\n"); 
            int sleep_time;
            unsigned int ret = sscanf(info, "%s %u", token, &sleep_time);
            printf("readBuffer: %s\n", readBuffer);
            printf("ret is %d\n", ret);
            if (ret == -1) {
                printf("sscanf failed\n");
            }
            printf("sleeping for %u\n", sleep_time);
            usleep(sleep_time * 1000);
        }
        else if (!strcmp(token, "stop\n")) {
            file_read_done = 1;
            printf("fileread complete\n");
        }
        
    }
    
}


void cpu_scheduler(char *alg) {
    
   
}


/*      List management functions */

/* ready q function */
void makeAndAddPCB(char* info, int newPID) {
    // Used by the fileread thread 
    
	// Storage for using strtok_r
	char *restOfString = info;
	char* token;
	// get space for the new PCB
	struct PCB* newPCB = (struct PCB*) malloc(sizeof(struct PCB));
	newPCB->pid = newPID;
	newPCB->cpuindex = 0;
	newPCB->ioindex = 0;
	// Skip the command type
	token = strtok_r(restOfString, " ", &restOfString);
	// The first token is the priority.
	token = strtok_r(restOfString, " ", &restOfString);
	newPCB->pr = atoi(token);
	// The second is the number of CPU/IO Bursts
	token = strtok_r(restOfString, " ", &restOfString);
	newPCB->numCPUBurst = (atoi(token) / 2) + 1;
	newPCB->numIOBurst = atoi(token) / 2;
	// Allocate space for the CPU and IO burst stingss
	newPCB->CPUBurst = (int *) (malloc(sizeof(int) * newPCB->numCPUBurst));
	newPCB->IOBurst = (int *) (malloc(sizeof(int) * newPCB->numIOBurst));
	// Time to insert each of the values into the CPU/IO arrays
	for(int i = 0; i < newPCB->numIOBurst; i++) {
		// First token in the pair is the CPU burst time
		token = strtok_r(restOfString, " ", &restOfString);
		newPCB->CPUBurst[i] = atoi(token);
		// Second token in the pair is the IO burst time
		token = strtok_r(restOfString, " ", &restOfString);
		newPCB->IOBurst[i] = atoi(token);
	}
	// The last token will just have a new line after it and not a space so we need to seperate it with a \n delimiter
	token = strtok_r(restOfString, "\n", &restOfString);
	newPCB->CPUBurst[newPCB->numCPUBurst - 1] = atoi(token);
	// TODO Set timspec begin here as well
	
	// Now we need to actually add the new PCB into the ready queue.
	readyAddPCB(newPCB);
}


/* ready q function */
void readyAddPCB(struct PCB* newPCB) {
    // Adds the given pcb to the ready_q. 
    // Called by the fileread thread, CPU, or io threads

    // First get the ready_q semaphore
    int ret = sem_wait(&ready_sem);
    if (ret) {
        printf("readyAddPCB problem");    
        printf("file read error getting sem: %s\n", strerror(errno));
        return;
    }
	// check the edge case of an unititialized RQ
	if(ready_q == NULL) {
		// Simply set the RQ to the new PCB and the prev and next to NULL.
		printf("Intializing RQ\n");
		ready_q = newPCB;
		newPCB->prev = NULL;
		newPCB->next = NULL;
        sem_post(&ready_sem);
		return;
	}
	// Otherwise we need to traverse to the back of the RQ and add it there.
	struct PCB* current = ready_q;
	while(current->next != NULL) {
		current = current->next;
	}
	// Now set current's previous to the new and the next of the new to the current
	current->next = newPCB;
	newPCB->next = NULL;
	newPCB->prev = current;
    // Release semaphore
    sem_post(&ready_sem);
}




/// Queue support functions
void printPCBs(struct PCB* queue) {
	struct PCB* current = queue;
	while(current != NULL) {
		printf("========================\n");
		printf("PID: %d\nPriority:%d\n", current->pid, current->pr);
		printf("CPU Burst times: size %d\n  ", current->numCPUBurst);
		for(int i = 0; i < current->numCPUBurst; i++)
			printf(" %d", current->CPUBurst[i]);
		printf("\nIO Burst times: size %d\n  ", current->numIOBurst);
                for(int i = 0; i < current->numIOBurst; i++)
                        printf(" %d", current->IOBurst[i]);
		printf("\nCurrent cpuindex: %d\nCurrent ioindex: %d\n", current->cpuindex, current->ioindex);
		// print start and end times
		current = current->next;
	}
	printf("\nend of PCBs\n");
}

void freePCBs(struct PCB* RQ) {
	while(RQ->next != NULL) {
		RQ = RQ->next;
		// Free the Burst arrays
		free(RQ->prev->CPUBurst);
		free(RQ->prev->IOBurst);
		free(RQ->prev);
	}
	free(RQ->CPUBurst);
	free(RQ->IOBurst);
	free(RQ);
}
