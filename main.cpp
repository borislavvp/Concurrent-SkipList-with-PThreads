/*
 * main.cpp
 *
 * Serial version
 *
 * Compile with -O2
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include <atomic>
#include <thread>
#include <pthread.h>
// #include "skiplist2.h"
// #include "queue.h"
#include "deque"
#include "skiplist.h"
#include "task_manager.h"

using namespace std;

task_manager manager;
void *workers_run(void *vargp)
{
    skiplist<int, int, 16> *list = (skiplist<int, int, 16> *)vargp;
    while (true)
    {
        manager.consumeInstructionWithWaiting(list);
    }
}
int main(int argc, char *argv[])
{
    struct timespec start, stop;
    skiplist<int, int, 16> list(0, 1000000);

    // check and parse command line options
    if (argc != 3)
    {
        printf("Q: Usage: %s <infile>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *fn = argv[1];
    int threads_number = atoi(argv[2]);

    pthread_t tid[threads_number];
    clock_gettime(CLOCK_REALTIME, &start);
    manager.initializeBarier(threads_number + 1);
    for (size_t i = 0; i < threads_number; i++)
    {
        int err = pthread_create(&tid[i], NULL, workers_run, (void *)&list);
        if (err)
        {
            printf("Q: ERROR; return code from pthread_create() is %d\n", err);
            exit(-1);
        }
    }

    // load input file
    FILE *fin = fopen(fn, "r");
    char action;
    long num;
    while (fscanf(fin, "%c %ld\n", &action, &num) > 0)
    {
        manager.publishInstruction(INSTRUCTION(action, num));
        if (action == 'p')
        {
            manager.waitWorkersForPrinting();
            cout << "Q: " << (list).printList() << endl;
        }
    }
    cout << "Q: DONE" << endl;
    manager.finalizeProcessing();
    for (size_t i = 0; i < threads_number; i++)
    {
        int err = pthread_join(tid[i], 0);
        if (err)
        {
            printf("Q: ERROR; return code from pthread_join() is %d\n", err);
        }
    }

    fclose(fin);
    clock_gettime(CLOCK_REALTIME, &stop);
    cout << "Q: COUNT" << manager.getInstructionsHandled() << endl;

    // print results
    double elapsed_time = (stop.tv_sec - start.tv_sec) + ((double)(stop.tv_nsec - start.tv_nsec)) / BILLION;

    cout << "Q: Elapsed time: " << elapsed_time << " sec" << endl;
    cout << "Q: Throughput: " << (double)manager.getInstructionsHandled() / elapsed_time << " ops (operations/sec)" << endl;

    return (EXIT_SUCCESS);
}