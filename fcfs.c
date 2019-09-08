/*
# Family Name: Cakmak

# Given Name: Ege

# Student Number: 215173131

# CSE Login: cakmake
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sch-helpers.h"

typedef int bool;
#define true 1
#define false 0

#define numberOfProcessors 4

int firstAvailableProcessor(process *CPUProcesses[4]);

bool areThereActiveProcesses(process *CPUProcesses[4]);

bool areThereAvailableProcessors(process *CPUProcesses[numberOfProcessors]);

int pidComparator(const void *a, const void *b);

int main() {
    process processes[MAX_PROCESSES + 1];
    int numberOfProcesses = 0;
    int status;
    int clock = 0;
    // Copies inputted process into the process array.
    while ((status = readProcess(&processes[numberOfProcesses]))) {
        if (status == 1) {
            numberOfProcesses++;
        }
    }
    // Sorts added processes based on their arrival time.
    qsort(processes, (size_t) numberOfProcesses, sizeof (process), compareByArrival);

    process_queue *readyQueue = malloc(sizeof (readyQueue));
    process_queue *deviceQueue = malloc(sizeof (deviceQueue));
    initializeProcessQueue(readyQueue);
    initializeProcessQueue(deviceQueue);
    /* This is the CPU. We will assign each CPU processes by inserting processes
    into this array. */
    process * CPUProcesses[4] = {NULL};
    // Loop invariants.
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    /* An array to store everything before letting them go in the ready queue.
    We will use this array to sort all processes going into that queue based on
    their process IDs.*/
    process * beforeReady[MAX_PROCESSES + 1];
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
            qsort(beforeReady, beforeReadyQueueSize, sizeof (process *), pidComparator);
        }

        // Transfers processes in beforeReady to ready queue and sets beforeReadyQueueSize to 0.
        for (l = 0; l < beforeReadyQueueSize; l++) {
            enqueueProcess(readyQueue, beforeReady[l]);
            beforeReady[l] = NULL;
        }
        beforeReadyQueueSize = 0;


        // A variable to store the first available CPU index.
        int freeProcessor;

        // Dispatches processes to available processors.
        while (readyQueue->size > 0 && areThereAvailableProcessors(CPUProcesses)) {
            process_node *ptr_1 = readyQueue->front;
            freeProcessor = firstAvailableProcessor(CPUProcesses);
            process *currentProcess = ptr_1->data;
            CPUProcesses[freeProcessor] = currentProcess;
            dequeueProcess(readyQueue);
            if (currentProcess->currentBurst == 0) currentProcess->startTime = clock;
        }
        // Increments waiting time of those processes that wait in the ready queue.
        process_node *ptr_1 = readyQueue->front;
        for (m = 0; m < readyQueue->size; m++) {
            ptr_1->data->waitingTime++;
            ptr_1 = ptr_1->next;
        }
        /* Increments steps and currentsBursts (if needed) of processes that are
         * doing their I/O Bursts.
         * Processes done doing I/O will be put back on a temp array to be sorted
         * then will be put on the ready queue.
         * Processes that are not done will be put back to the end of the queue. */
        int deviceQueueSize = deviceQueue->size;
        for (k = 0; k < deviceQueueSize; k++) {
            process *currentProcess = deviceQueue->front->data;
            dequeueProcess(deviceQueue);
            int currentBurst = currentProcess->currentBurst;
            currentProcess->bursts[currentBurst].step++;
            if (currentProcess->bursts[currentBurst].step == currentProcess->bursts[currentBurst].length) {
                currentProcess->currentBurst++;
                //                enqueueProcess(readyQueue,currentProcess);
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
                }
            }
        }
        // Breaks out of the loop when everything is done.
        if (deviceQueue->size == 0 && readyQueue->size == 0 && !areThereActiveProcesses(CPUProcesses) && beforeReadyQueueSize == 0 && i == numberOfProcesses) {
            break;
        }
        // Increments the CPU clock.
        clock++;

    }

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

    int contextSwitches = 0;

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


    return 0; // Return with no problems.
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

