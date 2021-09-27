/** needed because otherwise the max level is initialized multiple times as the file is included in multiple files */
#pragma once
#include <iostream>
#include <sstream>
#include <pthread.h>
#define BILLION 1000000000L

using namespace std;
;
template <class K, class V, int MAXLEVEL>
class skiplist_node
{
private:
    pthread_mutex_t nodeLock;

public:
    K key;
    V value;
    skiplist_node<K, V, MAXLEVEL> *forwards[MAXLEVEL + 1];

    skiplist_node()
    {
        for (int i = 1; i <= MAXLEVEL; i++)
        {
            forwards[i] = NULL;
        }
        nodeLock = PTHREAD_MUTEX_INITIALIZER;
    }

    skiplist_node(K searchKey) : key(searchKey)
    {
        for (int i = 1; i <= MAXLEVEL; i++)
        {
            forwards[i] = NULL;
        }
        nodeLock = PTHREAD_MUTEX_INITIALIZER;
    }

    skiplist_node(K searchKey, V val) : key(searchKey), value(val)
    {
        for (int i = 1; i <= MAXLEVEL; i++)
        {
            forwards[i] = NULL;
        }
        nodeLock = PTHREAD_MUTEX_INITIALIZER;
    }

    virtual ~skiplist_node()
    {
    }

    void lock()
    {
        pthread_mutex_trylock(&nodeLock);
    }
    void unlock()
    {
        pthread_mutex_unlock(&nodeLock);
    }
};

///////////////////////////////////////////////////////////////////////////////

template <class K, class V, int MAXLEVEL = 16>
class skiplist
{
protected:
    pthread_mutex_t listLock = PTHREAD_MUTEX_INITIALIZER;
    double uniformRandom()
    {
        return rand() / double(RAND_MAX);
    }

    int randomLevel()
    {
        int level = 1;
        double p = 0.5;
        while (uniformRandom() < p && level < MAXLEVEL)
        {
            level++;
        }
        return level;
    }
    K m_minKey;
    K m_maxKey;
    int max_curr_level;
    skiplist_node<K, V, MAXLEVEL> *m_pHeader;
    skiplist_node<K, V, MAXLEVEL> *m_pTail;

public:
    typedef K KeyType;
    typedef V ValueType;
    typedef skiplist_node<K, V, MAXLEVEL> NodeType;

    skiplist(K minKey, K maxKey) : m_pHeader(NULL), m_pTail(NULL),
                                   max_curr_level(1), max_level(MAXLEVEL),
                                   m_minKey(minKey), m_maxKey(maxKey)
    {
        m_pHeader = new NodeType(m_minKey);
        m_pTail = new NodeType(m_maxKey);
        for (int i = 1; i <= MAXLEVEL; i++)
        {
            m_pHeader->forwards[i] = m_pTail;
        }
    }

    virtual ~skiplist()
    {
        NodeType *tempNode;
        NodeType *currNode = m_pHeader->forwards[1];
        while (currNode != m_pTail)
        {
            tempNode = currNode;
            currNode = currNode->forwards[1];
            delete tempNode;
        }
        delete m_pHeader;
        delete m_pTail;
    }

    void insert(K searchKey, V newValue)
    {

        int newlevel = randomLevel();
        int level;
        int highestLocked;

        bool valid;

        skiplist_node<K, V, MAXLEVEL> *succs[MAXLEVEL];
        skiplist_node<K, V, MAXLEVEL> *preds[MAXLEVEL];
        NodeType *predNode = m_pHeader;
        NodeType *currNode;
        NodeType *succNode;

        while (true)
        {

            predNode = m_pHeader;
            for (level = MAXLEVEL; level >= 1; level--)
            {
                currNode = predNode->forwards[level];
                while (currNode->key < searchKey)
                {
                    predNode = currNode;
                    currNode = predNode->forwards[level];
                }
                preds[level] = predNode;
                succs[level] = currNode;
            }

            if (currNode->key == searchKey)
            {
                currNode->value = newValue;
                return;
            }

            highestLocked = 0;
            try
            {

                valid = true;
                if (newlevel > max_curr_level)
                {
                    max_curr_level = newlevel;
                }

                for (level = 1; valid && (level <= newlevel); level++)
                {
                    predNode = preds[level];
                    predNode->lock();
                    succNode = succs[level];
                    highestLocked = level;
                    valid = predNode->forwards[level] == succNode;
                }
                if (!valid)
                {
                    for (level = 1; level <= highestLocked; level++)
                    {
                        preds[level]->unlock();
                    }
                    continue;
                }

                currNode = new NodeType(searchKey, newValue);

                for (level = 1; level <= newlevel; level++)
                {
                    currNode->forwards[level] = preds[level]->forwards[level];
                    preds[level]->forwards[level] = currNode;
                }

                for (level = 1; level <= highestLocked; level++)
                {
                    preds[level]->unlock();
                }

                return;
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
                for (level = 1; level <= highestLocked; level++)
                {
                    preds[level]->unlock();
                }
            }
        }
    }

    void erase(K searchKey)
    {
        skiplist_node<K, V, MAXLEVEL> *update[MAXLEVEL];
        NodeType *currNode = m_pHeader;
        for (int level = max_curr_level; level >= 1; level--)
        {
            while (currNode->forwards[level]->key < searchKey)
            {
                currNode = currNode->forwards[level];
            }
            update[level] = currNode;
        }
        currNode = currNode->forwards[1];
        if (currNode->key == searchKey)
        {
            for (int lv = 1; lv <= max_curr_level; lv++)
            {
                if (update[lv]->forwards[lv] != currNode)
                {
                    break;
                }
                update[lv]->forwards[lv] = currNode->forwards[lv];
            }
            delete currNode;
            // update the max level
            while (max_curr_level > 1 && m_pHeader->forwards[max_curr_level] == NULL)
            {
                max_curr_level--;
            }
        }
    }

    //const NodeType* find(K searchKey)
    V find(K searchKey)
    {
        NodeType *currNode = m_pHeader;
        for (int level = max_curr_level; level >= 1; level--)
        {
            while (currNode->forwards[level]->key < searchKey)
            {
                currNode = currNode->forwards[level];
            }
        }
        currNode = currNode->forwards[1];
        if (currNode->key == searchKey)
        {
            return currNode->value;
        }
        else
        {
            //return NULL;
            return -1;
        }
    }

    bool empty() const
    {
        return (m_pHeader->forwards[1] == m_pTail);
    }

    std::string printList()
    {
        int i = 0;
        std::stringstream sstr;
        NodeType *currNode = m_pHeader->forwards[1];
        while (currNode != m_pTail)
        {
            //sstr << "(" << currNode->key << "," << currNode->value << ")" << endl;
            sstr << currNode->key << " ";
            currNode = currNode->forwards[1];
            i++;
            if (i > 200)
                break;
        }
        return sstr.str();
    }

    const int max_level;
};

///////////////////////////////////////////////////////////////////////////////
