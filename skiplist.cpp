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

#include "skiplist2.h"
//#include "skiplist.h"

using namespace std;

int main(int argc, char *argv[])
{
    int count = 0;
    struct timespec start, stop;

    skiplist<int, int> list(0, 1000000);

    // check and parse command line options
    if (argc != 3)
    {
        printf("Usage: %s <infile>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *fn = argv[1];

    clock_gettime(CLOCK_REALTIME, &start);

    // load input file
    FILE *fin = fopen(fn, "r");
    char action;
    long num;
    while (fscanf(fin, "%c %ld\n", &action, &num) > 0)
    {
        if (action == 'i')
        { // insert
            list.insert(num, num);
        }
        else if (action == 'q')
        { // qeury
            if (list.find(num) != num)
                cout << "ERROR: Not Found: " << num << endl;
        }
        else if (action == 'w')
        { // wait
            usleep(num);
        }
        else if (action == 'p')
        { // wait
            cout << list.printList() << endl;
        }
        else
        {
            printf("ERROR: Unrecognized action: '%c'\n", action);
            exit(EXIT_FAILURE);
        }
        count++;
    }
    fclose(fin);
    clock_gettime(CLOCK_REALTIME, &stop);

    // print results
    double elapsed_time = (stop.tv_sec - start.tv_sec) + ((double)(stop.tv_nsec - start.tv_nsec)) / BILLION;

    cout << "Q: COUNT" << count << endl;
    cout << "Elapsed time: " << elapsed_time << " sec" << endl;
    cout << "Throughput: " << (double)count / elapsed_time << " ops (operations/sec)" << endl;

    return (EXIT_SUCCESS);
}
