/*
# Family Name: Cakmak

# Given Name: Ege

# Student Number: 215173131

# CSE Login: cakmake
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include "sch-helpers.h"

typedef int bool;
#define true 1
#define false 0

#define numberOfProcessors 4

void checkAndRegisterInput(int argc, char *argv[], int *timeQuantum);

void printResults(process processes[], double totalCPUTime, int clock, int contextSwitches, int numberOfProcesses);

int firstAvailableProcessor(process *CPUProcesses[4]);

bool areThereActiveProcesses(process *CPUProcesses[4]);

bool areThereAvailableProcessors(process *CPUProcesses[numberOfProcessors]);

int pidComparator(const void *a, const void *b);

int main(int argc, char *argv[]) {

    int timeQuantum;

    checkAndRegisterInput(argc, argv, &timeQuantum);

    process processes[MAX_PROCESSES + 1];
    int numberOfProcesses = 0;
    int status;
    int clock = 0;
    int contextSwitches = 0;

    // Copies inputted process into the process array.
    while ((status = readProcess(&processes[numberOfProcesses]))) {
        if (status == 1) {
            numberOfProcesses++;
        }
    }

    // Sorts added processes based on their arrival time.
    qsort(processes, (size_t) numberOfProcesses, sizeof(process), compareByArrival);

    process_queue *readyQueue = malloc(sizeof(readyQueue));
    process_queue *deviceQueue = malloc(sizeof(deviceQueue));
    initializeProcessQueue(readyQueue);
    initializeProcessQueue(deviceQueue);

    /* This is the CPU. We will assign each CPU processes by inserting processes
    into this array. */
    process *CPUProcesses[4] = {NULL};
    // Loop invariants.
    int i = 0;
    int j = 0;

    /* An array to store everything before letting them go in the ready queue.
    We will use this array to sort all processes going into that queue based on
    their process IDs.*/
    process *beforeReady[MAX_PROCESSES + 1];
    size_t beforeReadyQueueSize = 0;
    // A variable to store total time CPU will be run
    double totalCPUTime = 0;
    while (true) {
        // Inserts arriving processes to beforeReady.
        while (i < numberOfProcesses) {
            if (processes[i].arrivalTime <= clock) {
                beforeReady[beforeReadyQueueSize++] = &processes[i];
                i++;
            } else {
                break;
            }
        }
        /* Sorts everything in beforeReady by their process ID.
         * beforeReady includes both arriving processes and the ones that are
         * done doing IO.*/
        if (beforeReadyQueueSize > 1) {
            qsort(beforeReady, beforeReadyQueueSize, sizeof(process * ), pidComparator);
        }

        // Transfers processes in beforeReady to ready queue and sets beforeReadyQueueSize to 0.
        for (j = 0; j < beforeReadyQueueSize; j++) {
            enqueueProcess(readyQueue, beforeReady[j]);
            beforeReady[j] = NULL;
        }
        beforeReadyQueueSize = 0;


        // A variable to store the first available CPU index.
        int freeProcessor;

        // Dispatches processes to available processors.
        while (readyQueue->size > 0 && areThereAvailableProcessors(CPUProcesses)) {
            process_node *ptr_1 = readyQueue->front;
            freeProcessor = firstAvailableProcessor(CPUProcesses);
            process *currentProcess = ptr_1->data;
            currentProcess->quantumRemaining = timeQuantum;
            CPUProcesses[freeProcessor] = currentProcess;
            dequeueProcess(readyQueue);
            if (currentProcess->currentBurst == 0 &&
                currentProcess->bursts[currentProcess->currentBurst].step == 0)
                currentProcess->startTime = clock;
        }

        // Increments waiting time of those processes that are waiting in the ready queue.
        process_node *ptr_1 = readyQueue->front;
        for (j = 0; j < readyQueue->size; j++) {
            ptr_1->data->waitingTime++;
            ptr_1 = ptr_1->next;
        }

        /* Increments steps and currentsBursts (if needed) of processes that are
         * doing their I/O Bursts.
         * Processes done doing I/O will be put back on a temp array to be sorted
         * then will be put on the ready queue.
         * Processes that are not done will be put back to the end of the queue. */
        int deviceQueueSize = deviceQueue->size;
        for (j = 0; j < deviceQueueSize; j++) {
            process *currentProcess = deviceQueue->front->data;
            dequeueProcess(deviceQueue);
            int currentBurst = currentProcess->currentBurst;
            currentProcess->bursts[currentBurst].step++;
            if (currentProcess->bursts[currentBurst].step == currentProcess->bursts[currentBurst].length) {
                currentProcess->currentBurst++;
                beforeReady[beforeReadyQueueSize] = currentProcess;
                beforeReadyQueueSize++;

            } else {
                enqueueProcess(deviceQueue, currentProcess);
            }
        }
        /* Processes the running processes. 
         * The reason we have this after processDeviceQueue is to prevent processes
         * from doing their I/O bursts in the same clock cycle they did their CPU Bursts.
         * Changing the order does not affect functionality. */
        for (j = 0; j < numberOfProcessors; j++) {
            if (CPUProcesses[j] != NULL) {
                totalCPUTime += 100;
                process *currentProcess = CPUProcesses[j];
                currentProcess->quantumRemaining--; // Decrements the time quantum of the process.
                int currentBurst = currentProcess->currentBurst;
                if (currentProcess->bursts[currentBurst].step < currentProcess->bursts[currentBurst].length) {
                    currentProcess->bursts[currentBurst].step++;
                }
                if (currentProcess->bursts[currentBurst].step == currentProcess->bursts[currentBurst].length) {
                    currentProcess->currentBurst++;
                    if (currentProcess->currentBurst == currentProcess->numberOfBursts) {
                        currentProcess->endTime = clock;
                    } else {
                        enqueueProcess(deviceQueue, currentProcess);
                    }
                    CPUProcesses[j] = NULL;
                    // Preempt the process if its not done within the given time.
                } else if (currentProcess->quantumRemaining == 0) {
                    beforeReady[beforeReadyQueueSize] = currentProcess;
                    contextSwitches++;
                    beforeReadyQueueSize++;
                    CPUProcesses[j] = NULL;
                }
            }
        }

        // Breaks out of the loop when everything is done.
        if (deviceQueue->size == 0 &&
            readyQueue->size == 0 &&
            !areThereActiveProcesses(CPUProcesses) &&
            beforeReadyQueueSize == 0 &&
            i == numberOfProcesses) {
            break;
        }

        // Increments the CPU clock.
        clock++;

    }

    printResults(processes, totalCPUTime, clock, contextSwitches, numberOfProcesses);

    return 0; // Return with no problems.
}

void checkAndRegisterInput(int argc, char *argv[], int *timeQuantum) {
    if (argc < 2 || argc > 2) {
        fprintf(stderr,
                "USAGE: ./rr TIME-QUANTUM < CPU-LOAD-FILE eg. \"./rr 8 < load\" for time quantum 8 units processing workload in file 'load'\n");
    	exit(1);
    }


    char *temp;
    errno = 0;
    long convTemp = strtol(argv[1], &temp, 10);

    if (errno != 0 || *temp != '\0' || convTemp > INT_MAX) {
        fprintf(stderr, "Error reading time quantum.\n");
    	exit(1);
    } else {
        *timeQuantum = convTemp;
    }

    if (*timeQuantum < 1 || *timeQuantum > (powf(2, 31) - 1)) {
        fprintf(stderr, "Invalid time quantum: expected integer in range [1,2^31-1].\n");
    	exit(1);
    }

}

void printResults(process processes[], double totalCPUTime, int clock, int contextSwitches, int numberOfProcesses) {
    int i;
    double waitingTimeSum = 0;
    for (i = 0; i < numberOfProcesses; i++) {
        waitingTimeSum = waitingTimeSum + processes[i].waitingTime;
    }
    double avgWaitingTime = (double) waitingTimeSum / (double) numberOfProcesses;

    double turnaroundTimeSum = 0;
    for (i = 0; i < numberOfProcesses; i++) {
        int endTime = processes[i].endTime;
        int arrivalTime = processes[i].arrivalTime;
        int turnaroundTime = endTime - arrivalTime + 1;
        turnaroundTimeSum += turnaroundTime;
    }
    double avgTurnaroundTime = (double) turnaroundTimeSum / (double) numberOfProcesses;

    int totalTime = clock + 1;
    double averageCPUUtilization = totalCPUTime / (double) totalTime;



    // Gets the most time that was taken by a process.
    int x;
    int maxTime = 0;
    for (x = 0; x < numberOfProcesses; x++) {
        if (processes[x].endTime > maxTime) {
            maxTime = processes[x].endTime;
        }
    }

    printf("Average waiting time                 : %.2f units\n", avgWaitingTime);
    printf("Average turnaround time              : %.2f units\n", avgTurnaroundTime);
    printf("Time all processes finished          : %d\n", totalTime);
    printf("Average CPU utilization              : %.1f %% \n", averageCPUUtilization);
    printf("Number of context switches           : %d\n", contextSwitches);
    printf("PID(s) of last process(es) to finish : ");
    // Prints the last ending pids.
    for (x = 0; x < numberOfProcesses; x++) {
        if (processes[x].endTime == maxTime) {
            printf("%d ", processes[x].pid);
        }
    }
    printf("\n");

}

int firstAvailableProcessor(process *CPUProcesses[numberOfProcessors]) {
    int i;
    for (i = 0; i < numberOfProcessors; i++) {
        if (CPUProcesses[i] == NULL) {
            return i;
        }
    }
    return -1;
}

bool areThereActiveProcesses(process *CPUProcesses[numberOfProcessors]) {
    int i;
    for (i = 0; i < numberOfProcessors; i++) {
        if (CPUProcesses[i] != NULL) {
            return true;
        }
    }
    return false;
}

bool areThereAvailableProcessors(process *CPUProcesses[numberOfProcessors]) {
    int result = firstAvailableProcessor(CPUProcesses);
    if (result >= 0) return true;
    else
        return false;
}

int pidComparator(const void *a, const void *b) {
    return (*(process **) a)->pid - (*(process **) b)->pid;
}

