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

//// general
void print_usage();
void *fileread();       // file read thread
void *cpu_process();    // cpu thread

void cpu_sim(struct PCB*); // cpu_process helper

////// list management
/* helpers */
void printPCBs(struct PCB*);
void freePCBs(struct PCB*);
void addDoneQ(struct PCB*);
/* ready q */
void makeAndAddPCB(char* info, int newPID);
void readyAddPCB(struct PCB*);
struct PCB* readyRemovePCB(char *);
/* io q */
void addIOq(struct PCB*);


sem_t ready_sem;  
sem_t io_sem;

struct PCB *ready_q = NULL;
struct PCB *done_list = NULL;  // Only the cpu thread touches this. Stores completed processes for later evaluation
struct PCB *io_q = NULL;


// State trackers
// updated to 1 upon completion
int file_read_done = 0;  
int cpu_sch_done = 0;
int cpu_busy = 0;

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
    if (pthread_create(&fileread_thread, NULL, (*fileread), (void *)filepath)) {
        printf("Failed to create file read thread\n");
        exit(-1);
    }
    // cpu thread
    pthread_t cpu_thread;
    
    if (pthread_create(&cpu_thread, NULL, (*cpu_process), (void *)argv[2])) {
        printf("Failed to create cpu thread\n");
    }
    // Creating CPU thread

    // Awaiting threads
    pthread_join(fileread_thread, NULL);
    pthread_join(cpu_thread, NULL);

    
    printPCBs(ready_q);
    printf("********************************\n");
    printPCBs(io_q);
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
            printf("fileread complete\n");
            break;
        }
    }
    file_read_done = 1;
}


void *cpu_process(char *alg) {
    // Called from main
    while (1) {
        if (file_read_done && !cpu_busy && ready_q == NULL) {
            // Work is done 
            cpu_sch_done = 1; 
            break;
        }
        //printf("cpu asking for ready sem\n");
        if (sem_wait(&ready_sem)) {
            printf("Error asking for sem in cpu process\n"); 
            continue;
        }
        //printf("cpu has ready sem\n");
        // We have the semaphore 
        cpu_busy = 1;
        struct PCB* current_proc = readyRemovePCB(alg);
        if (current_proc == NULL) {
            // ready q empty.
            sem_post(&ready_sem);
            cpu_busy = 0;
            continue;
        }
        printf("we have proc number: %d\n", current_proc->pid);
        printf("with cpu index: %d\n", current_proc->cpuindex);
        // Selecting algorithm handler function
        cpu_sim(current_proc);
        //printf("cpu asking for io sem\n");
        // Adding proc to the io q
        if (sem_wait(&io_sem)) {
            printf("cpu process failed to get io sem\n");
            sem_post(&ready_sem);
            continue;
        }
        addIOq(current_proc); 
        cpu_busy = 0;
        sem_post(&io_sem);
        sem_post(&ready_sem);
        
        
        
    } 
    printf("cpu process is done\n");
     
}

void cpu_sim(struct PCB* current_proc) {
    // Simulates the cpu 
    // semaphore already grabbed before getting here
    printf("In cpu sim\n");
    int cpuindex = current_proc->cpuindex; 
    printf("index is %d\n", cpuindex);
    unsigned int sleep_time = current_proc->CPUBurst[cpuindex] * 1000;
    printf("cpu sim sleeping\n");
    usleep(sleep_time);
    current_proc->cpuindex++;
    // @TODO update the performance stats in the pcb
    if (current_proc->cpuindex >= current_proc->numCPUBurst) {
        // add to the done_list
        printf("Adding to done q\n");
        addDoneQ(current_proc); 
    }
    
    printf("Returning from cpu sim\n"); 
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
    // Called by the fileread thread and io thread

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
        printf("rq head pid: %d\n", ready_q->pid);
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


/* ready q function */
struct PCB* readyRemovePCB(char *alg) {
    // Returns the relevant PCB based on the given algorithm
    // Only called by cpu thread. Semaphore is already taken to get here.
   

    if (ready_q == NULL) {
        return NULL;
    }

    struct PCB* next_proc;
    // Selecting appropriate algorithm.
    if (!strcmp("FIFO", alg)) {
        printf("ready remove FIFO\n");
        // Popping pcb from queue
        next_proc = ready_q;
        // Advancing queue head to next pcb
        printf("Attempting to access ready_q next\n");
        ready_q = ready_q->next;
        printf("successfully  accessed ready_q next\n");
        // severing list
        next_proc->next = NULL;
        next_proc->prev = NULL;
        if (ready_q != NULL) {
            ready_q->prev = NULL;
        }
        printf("Null assignment in ready remove ok\n");
    }
    else {
        printf("Found the problem\n");
    }
    printf("Returning from ready remove\n");
    return next_proc;
}


/********  io_q functions **********/
void addIOq(struct PCB* proc) {
    // Expect caller to have the relevant semaphore first
    // Updating proc prev/next values just in case 
    printf("Adding proc %d to io q\n", proc->pid); 
    proc->next = NULL;
    proc->prev = NULL;
    if (io_q == NULL) {
        io_q = proc;
    }
    else {
        // finding end of the queue
        struct PCB* temp = io_q;
        while (temp->next != NULL) {
            temp = temp->next; 
        }
        temp->next = proc;
        proc->prev = temp;
    }
}


/// Queue support functions
void addDoneQ(struct PCB* proc) {
    // Order of the queue probably doesn't matter, so we'll just put each
    // process at the head of the queue
    // prev, next should already have been reset before arriving
    if (done_list == NULL) {
        done_list = proc;
    }
    else {
        proc->next = done_list; 
        done_list = proc;
    }
}

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
