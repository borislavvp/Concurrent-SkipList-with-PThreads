#include <iostream>
#include <sstream>

#include <pthread.h>
#include <thread>
#include <queue>
#include <deque>
#include <atomic>
#include "skiplist.h"
#include "instruction_handler.h"

using namespace std;
typedef pair<char, int> INSTRUCTION;
class task_manager
{
private:
    instruction_handler handler;
    std::vector<INSTRUCTION> task_queue;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
    pthread_barrier_t barrier;
    std::atomic<bool> printing;
    std::atomic<bool> done;

public:
    task_manager()
    {
        done = false;
        printing = false;
    }
    virtual ~task_manager()
    {
        task_queue.clear();
        pthread_mutex_destroy(&lock);
        pthread_barrier_destroy(&barrier);
        pthread_cond_destroy(&queue_cond);
    }
    void publishInstruction(INSTRUCTION item)
    {
        pthread_mutex_lock(&lock);
        if (item.first == 'p')
        {
            printing = true;
        }
        else
        {
            task_queue.push_back(item);
        }
        // cout << "publish" << endl;
        pthread_cond_signal(&queue_cond);
        pthread_mutex_unlock(&lock);
    }

    void consumeInstructionWithWaiting(skiplist<int, int> *list)
    {
        pthread_mutex_lock(&lock);
        while (task_queue.empty() && !done.load() && !printing.load())
        {
            pthread_cond_wait(&queue_cond, &lock);
        }

        if (printing.load() && task_queue.empty())
        {
            pthread_cond_signal(&queue_cond);
            pthread_mutex_unlock(&lock);
            pthread_barrier_wait(&barrier);
            usleep(5);
            return;
        }
        if (done.load() && task_queue.empty())
        {
            // cout << "out" << endl;
            pthread_cond_signal(&queue_cond);
            pthread_mutex_unlock(&lock);
            pthread_exit(0);
        }

        __attribute((aligned(128))) std::vector<INSTRUCTION> task_queue_batch;
        std::swap(task_queue, task_queue_batch);
        pthread_cond_signal(&queue_cond);
        pthread_mutex_unlock(&lock);

        // process all items from task_queue_batch in batch
        for (auto &item : task_queue_batch)
        {
            handler.handleInstruction(item.first, item.second, list, &barrier);
        }
        task_queue_batch.clear();
    }
    void initializeBarier(int numberOfWorkers)
    {
        pthread_barrier_init(&barrier, NULL, numberOfWorkers);
    }
    void waitWorkersForPrinting()
    {
        pthread_barrier_wait(&barrier);
        printing = false;
        pthread_cond_signal(&queue_cond);
        handler.increaseNumberOfInstructionsHandled();
    }
    int getInstructionsHandled()
    {
        return handler.getInstructionsHandled();
    }

    void finalizeProcessing()
    {
        done = true;
        pthread_cond_signal(&queue_cond);
    }
};
// class task_manager
// {
// private:
//     instruction_handler handler;
//     queue<INSTRUCTION> task_queue;
//     pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
//     pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
//     pthread_barrier_t barrier;
//     std::atomic<bool> done;
//     std::atomic<bool> printing;

// public:
//     task_manager()
//     {
//         done = false;
//         printing = false;
//     }
//     virtual ~task_manager()
//     {
//         pthread_mutex_destroy(&lock);
//         pthread_barrier_destroy(&barrier);
//         pthread_cond_destroy(&queue_cond);
//     }
//     void publishInstruction(INSTRUCTION item)
//     {
//         pthread_mutex_lock(&lock);
//         if (item.first == 'p')
//         {
//             printing = true;
//         }
//         else
//         {
//             task_queue.push(item);
//         }
//         // cout << "publish" << endl;
//         pthread_cond_signal(&queue_cond);
//         pthread_mutex_unlock(&lock);
//     }

//     void consumeInstructionWithWaiting(skiplist<int, int> *list)
//     {
//         pthread_mutex_lock(&lock);
//         while (task_queue.empty() && !done.load() && !printing)
//         {
//             pthread_cond_wait(&queue_cond, &lock);
//         }

//         if (printing.load())
//         {
//             pthread_cond_signal(&queue_cond);
//             pthread_mutex_unlock(&lock);
//             pthread_barrier_wait(&barrier);
//             usleep(5);
//             return;
//         }

//         if (done.load() && task_queue.empty())
//         {
//             // cout << "out" << endl;
//             pthread_cond_signal(&queue_cond);
//             pthread_mutex_unlock(&lock);
//             pthread_exit(0);
//         }

//         INSTRUCTION item = task_queue.front();
//         task_queue.pop();

//         pthread_cond_signal(&queue_cond);
//         pthread_mutex_unlock(&lock);

//         handler.handleInstruction(item.first, item.second, list, &barrier);
//     }
//     void initializeBarier(int numberOfWorkers)
//     {
//         pthread_barrier_init(&barrier, NULL, numberOfWorkers);
//     }
//     void waitWorkersForPrinting()
//     {
//         pthread_barrier_wait(&barrier);
//         printing = false;
//         pthread_cond_signal(&queue_cond);
//         handler.increaseNumberOfInstructionsHandled();
//     }
//     int getInstructionsHandled()
//     {
//         return handler.getInstructionsHandled();
//     }

//     void finalizeProcessing()
//     {
//         done = true;
//         pthread_cond_signal(&queue_cond);
//     }
// };
