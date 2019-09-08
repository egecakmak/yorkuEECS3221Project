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

void checkAndRegisterInput(int argc, char *argv[], int *q1tq, int *q2tq);

void incrementWaitingTimes(process_queue *q1, process_queue *q2, process_queue *q3);

void printResults(process processes[], double totalCPUTime, int clock, int contextSwitches, int numberOfProcesses);

int organizeCPUProcesses(process *CPUProcesses[numberOfProcessors]);

int availableNumberOfCPUS(process *CPUProcesses[numberOfProcessors]);

int firstAvailableProcessor(process *CPUProcesses[4]);

bool areThereActiveProcesses(process *CPUProcesses[4]);

bool areThereAvailableProcessors(process *CPUProcesses[numberOfProcessors], int queueType);

int pidComparator(const void *a, const void *b);

int queueComparator(const void *a, const void *b);

void addToBeginning(process *proc, process_queue *pq);

int main(int argc, char *argv[]) {

    int q1tq;
    int q2tq;
    checkAndRegisterInput(argc, argv, &q1tq, &q2tq);

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

    // Create and initialize needed queues.
    process_queue *q1 = malloc(sizeof(q1)); // RR TQ1
    process_queue *q2 = malloc(sizeof(q2)); // RR TQ1
    process_queue *q3 = malloc(sizeof(q3)); // FCFS
    process_queue *deviceQueue = malloc(sizeof(deviceQueue)); // Wait queue
    initializeProcessQueue(q1);
    initializeProcessQueue(q2);
    initializeProcessQueue(q3);
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

    // A variable to store total CPU time.
    double totalCPUTime = 0;

    while (true) {
        // Inserts arriving processes to beforeReady.
        while (i < numberOfProcesses) {
            if (processes[i].arrivalTime <= clock) {
                processes[i].currentQueue = 1;
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

        // Transfers processes in beforeReady to their corresponding queues.
        for (j = 0; j < beforeReadyQueueSize; j++) {
            if (beforeReady[j]->currentQueue == 1) {
                enqueueProcess(q1, beforeReady[j]);

            } else if (beforeReady[j]->currentQueue == 2) {
                enqueueProcess(q2, beforeReady[j]);

            } else if (beforeReady[j]->currentQueue == 3) {
                enqueueProcess(q3, beforeReady[j]);
            }

            beforeReady[j] = NULL;
        }
        beforeReadyQueueSize = 0; // Resets beforeReadyQueueSize.

        /* Dispatches processes in queue 1 to available processors. Preempts 
         * processors if possible. (i.e. processor runs a process that belongs 
         * to a higher level queue.) */
        while (q1->size && areThereAvailableProcessors(CPUProcesses, 1)) {
            /* Places processes back to their source array such that there are 
             * no nulls in 
             * between them. Returns the number of processes in the array.*/
            int activeProcesses = organizeCPUProcesses(CPUProcesses);

            /* Sorts the organized CPUProcess array. (Processes with higher
             * queue level come first. If queue levels are equal then the one
             * with higher pid comes first.*/
            if (activeProcesses > 1) {
                qsort(CPUProcesses, activeProcesses, sizeof(process * ), queueComparator);
            }

            /* Frees processors that are running processes belonging to a higher
             * level queue. Adds preempted processes to the front of their
             * corresponding queue. */
            for (j = 0; j < numberOfProcessors
                        && availableNumberOfCPUS(CPUProcesses) < q1->size; j++) {
                if (CPUProcesses[j] != NULL) {
                    if (CPUProcesses[j]->currentQueue == 3) {
                        addToBeginning(CPUProcesses[j], q3);
                        CPUProcesses[j] = NULL;
                        contextSwitches++;
                    } else if (CPUProcesses[j]->currentQueue == 2) {
                        addToBeginning(CPUProcesses[j], q2);
                        CPUProcesses[j] = NULL;
                        contextSwitches++;
                    }
                }
            }

            int freeProcessor = firstAvailableProcessor(CPUProcesses);
            process *currentProcess = q1->front->data;
            if (currentProcess->quantumRemaining == 0) {
                currentProcess->quantumRemaining = q1tq;
            }
            if (currentProcess->currentBurst == 0 &&
                currentProcess->bursts[currentProcess->currentBurst].step == 0)
                currentProcess->startTime = clock;
            CPUProcesses[freeProcessor] = currentProcess;
            dequeueProcess(q1);
        }

        /* Dispatches processes in queue 1 to avaiable processors. Preempts 
         * processors if possible. (i.e. processor runs a process that belongs 
         * to a higher level queue.) */

        while (q2->size && areThereAvailableProcessors(CPUProcesses, 2)) {
            /* Places processes back to their source array such that there are 
             * no nulls in 
             * between them. Returns the number of processes in the array.*/
            int activeProcesses = organizeCPUProcesses(CPUProcesses);

            /* Sorts the organized CPUProcess array. (Processes with higher
             * queue level come first. If queue levels are equal then the one
             * with higher pid comes first.*/
            if (activeProcesses > 1) {
                qsort(CPUProcesses, activeProcesses, sizeof(process * ), queueComparator);
            }

            /* Frees processors that are running processes belonging to a higher
             * level queue.Adds preempted processes to the front of their
             * corresponding queue. */
            for (j = 0; j < numberOfProcessors && availableNumberOfCPUS(CPUProcesses) < q2->size; j++) {
                if (CPUProcesses[j] != NULL) {
                    if (CPUProcesses[j]->currentQueue == 3) {
                        addToBeginning(CPUProcesses[j], q3);
                        CPUProcesses[j] = NULL;
                        contextSwitches++;
                    }
                }
            }
            int freeProcessor = firstAvailableProcessor(CPUProcesses);
            process *currentProcess = q2->front->data;
            if (currentProcess->quantumRemaining == 0) {
                currentProcess->quantumRemaining = q2tq;
            }
            if (currentProcess->currentBurst == 0 &&
                currentProcess->bursts[currentProcess->currentBurst].step == 0)
                currentProcess->startTime = clock;
            CPUProcesses[freeProcessor] = currentProcess;
            dequeueProcess(q2);
        }

        // Dispatches processes in queue 1 to available processors.
        while (q3->size && areThereAvailableProcessors(CPUProcesses, 3)) {
            int freeProcessor = firstAvailableProcessor(CPUProcesses);
            process *currentProcess = q3->front->data;
            /* Assigning -1 as the quantum time for this process since we
             * do not need it as this queue is simply a FCFS queue.
             */
            currentProcess->quantumRemaining = -1;
            if (currentProcess->currentBurst == 0 &&
                currentProcess->bursts[currentProcess->currentBurst].step == 0)
                currentProcess->startTime = clock;
            CPUProcesses[freeProcessor] = currentProcess;
            dequeueProcess(q3);
        }

        incrementWaitingTimes(q1, q2, q3);

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
         * The reason we have this after doing IO bursts is to prevent processes
         * from doing their I/O bursts in the same clock cycle they did their CPU Bursts.
         * Changing the order does not affect functionality. */
        for (j = 0; j < numberOfProcessors; j++) {
            if (CPUProcesses[j] != NULL) {
                totalCPUTime += 100; // Increases total CPU time for the calculations.
                process *currentProcess = CPUProcesses[j];
                currentProcess->quantumRemaining--; // Decreases quantum time of the process.
                int currentBurst = currentProcess->currentBurst;
                if (currentProcess->bursts[currentBurst].step < currentProcess->bursts[currentBurst].length) {
                    currentProcess->bursts[currentBurst].step++;
                }
                if (currentProcess->bursts[currentBurst].step == currentProcess->bursts[currentBurst].length) {
                    currentProcess->currentBurst++;
                    if (currentProcess->currentBurst == currentProcess->numberOfBursts) {
                        currentProcess->endTime = clock;
                    } else {
                        currentProcess->currentQueue = 1;
                        currentProcess->quantumRemaining = 0;
                        enqueueProcess(deviceQueue, currentProcess);
                    }
                    CPUProcesses[j] = NULL;
                    // Preempt the process if its not done within the given time.
                } else if (currentProcess->quantumRemaining == 0) {
                    currentProcess->currentQueue++;
                    beforeReady[beforeReadyQueueSize] = currentProcess;
                    contextSwitches++;
                    beforeReadyQueueSize++;
                    CPUProcesses[j] = NULL;
                }
            }
        }

        // Breaks out of the loop when everything is done.
        if (deviceQueue->size == 0 &&
            q1->size == 0 &&
            q2->size == 0 &&
            q3->size == 0 &&
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

void checkAndRegisterInput(int argc, char *argv[], int *q1tq, int *q2tq) {
    if (argc < 3 || argc > 3) {
        fprintf(stderr, "USAGE: ./fbq QUANTUM1 QUANTUM2 < CPU-LOAD-FILE eg. "
                        "\"./fbq 10 30 < load\" for a 3-level feedback queue "
                        "wherein Q1 has time "
                        "quantum 10, Q2 has quantum 30, and Q3 is FCFS, "
                        "processing workload in file 'load'\n");
        exit(1); // Input error.
    }

    char *temp;
    char *temp2;
    errno = 0;
    long convTemp = strtol(argv[1], &temp, 10);

    if (errno != 0 || *temp != '\0' || convTemp > INT_MAX) {
        fprintf(stderr, "Error reading time quantum 1.\n");
        exit(1); // Input error.
    } else {
        *q1tq = convTemp;
    }

    convTemp = strtol(argv[2], &temp2, 10);

    if (errno != 0 || *temp2 != '\0' || convTemp > INT_MAX) {
        fprintf(stderr, "Error reading time quantum 2.\n");
        exit(1); // Input error.
    } else {
        *q2tq = convTemp;
    }

    if (*q1tq < 1 || *q1tq > (powf(2, 31) - 1)) {
        fprintf(stderr, "Invalid time quantum (1): expected"
                        " integer in range [1,2^31-1].\n");
        exit(1); // Input error.
    }

    if (*q2tq < 1 || *q2tq > (powf(2, 31) - 1)) {
        fprintf(stderr, "Invalid time quantum (2): expected"
                        " integer in range [1,2^31-1].\n");
        exit(1); // Input error.
    }

}

/* Increments waiting times of all processes waiting in the queues. 
 * (Except for the device queue.) */
void incrementWaitingTimes(process_queue *q1, process_queue *q2, process_queue *q3) {
    int m;
    process_node *ptr_1 = q1->front;
    for (m = 0; m < q1->size; m++) {
        ptr_1->data->waitingTime++;
        ptr_1 = ptr_1->next;
    }
    process_node *ptr_2 = q2->front;
    for (m = 0; m < q2->size; m++) {
        ptr_2->data->waitingTime++;
        ptr_2 = ptr_2->next;
    }
    process_node *ptr_3 = q3->front;
    for (m = 0; m < q3->size; m++) {
        ptr_3->data->waitingTime++;
        ptr_3 = ptr_3->next;
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

/* Places processes back to their source array such that there are no nulls in 
 * between them. Returns the number of processes in the array.*/
int organizeCPUProcesses(process *CPUProcesses[numberOfProcessors]) {
    process *temp[numberOfProcessors] = {NULL};
    int tempSize = 0;
    int i;
    for (i = 0; i < numberOfProcessors; i++) {
        if (CPUProcesses[i] != NULL) {
            temp[tempSize++] = CPUProcesses[i];
        }
        CPUProcesses[i] = NULL;
    }
    for (i = 0; i < tempSize; i++) {
        CPUProcesses[i] = temp[i];
    }
    return tempSize;
}

int availableNumberOfCPUS(process *CPUProcesses[numberOfProcessors]) {
    int i, counter = 0;
    for (i = 0; i < numberOfProcessors; i++) {
        if (CPUProcesses[i] == NULL) {
            counter++;
        }
    }
    return counter;
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

/* Returns the number of available processors. Counts processors that can
 be preempted as well.*/
bool areThereAvailableProcessors(process *CPUProcesses[numberOfProcessors], int queueType) {
    int i;
    for (i = 0; i < numberOfProcessors; i++) {
        if (CPUProcesses[i] == NULL || CPUProcesses[i]->currentQueue > queueType) {
            return true;
        }
    }
    return false;
}

int pidComparator(const void *a, const void *b) {
    return (*(process **) a)->pid - (*(process **) b)->pid;
}

int queueComparator(const void *a, const void *b) {
    process *proca = (*(process **) a);
    process *procb = (*(process **) b);
    if (proca->currentQueue > procb->currentQueue) {
        return -1;
    } else if (proca->currentQueue < procb->currentQueue) {
        return 1;
    } else {
        return -1 * pidComparator(a, b);
    }
}

/* A simple linked list function that adds the given element to the front 
 * of the queue.*/
void addToBeginning(process *proc, process_queue *pq) {
    process_node *node = createProcessNode(proc);

    if (pq->front == NULL) {
        pq->front = pq->back = node;
    } else {
        node->next = pq->front;
        if (pq->front->next == NULL) {
            pq->back = pq->front;
        }
        pq->front = node;
    }
    pq->size++;

}
