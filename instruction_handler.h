#include <iostream>
#include <sstream>

#include <pthread.h>
#include <thread>
#include <deque>
#include <atomic>
#include "skiplist.h"

using namespace std;
typedef pair<char, int> INSTRUCTION;
class instruction_handler
{
private:
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

public:
    std::atomic<int> count{0};
    void handleInstruction(char action, int num, skiplist<int, int> *list, pthread_barrier_t *barrier)
    {
        if (action == 'i')
        { // insert
            (*list).insert(num, num);
        }
        else if (action == 'q')
        { // qeury
            if ((*list).find(num) != num)
                cout << "Q: ERROR: Not Found: " << num << endl;
        }
        else if (action == 'w')
        { // wait
            usleep(num);
        }
        else
        {
            printf("Q: ERROR: Unrecognized action: '%c'\n", action);
            exit(EXIT_FAILURE);
        }

        increaseNumberOfInstructionsHandled();
    }

    int getInstructionsHandled()
    {
        return count;
    }
    void increaseNumberOfInstructionsHandled()
    {
        count += 1;
    }
};
