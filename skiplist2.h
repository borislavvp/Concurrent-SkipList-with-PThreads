#include <iostream>
#include <sstream>

#define BILLION 1000000000L
#define NPAIRS 44

using namespace std;

template <class K, class V, int MAXLEVEL>
class skiplist_node
{
public:
    int cnt;
    // change KV to array of structure later
    K key[NPAIRS];                                         // 4*44   --> 176
    V value[NPAIRS];                                       // 4*44   --> 176
    skiplist_node<K, V, MAXLEVEL> *forwards[MAXLEVEL + 1]; // 8*17 = 156 bytes --> 128 + 28 bytes
    // total 352 + 128 + 28 + 4 -->  512 bytes --> 8 cachelines
    skiplist_node()
    {
        for (int i = 1; i <= MAXLEVEL; i++)
        {
            forwards[i] = NULL;
        }
        cnt = 0;
    }

    skiplist_node(K searchKey)
    {
        for (int i = 1; i <= MAXLEVEL; i++)
        {
            forwards[i] = NULL;
        }
        key[0] = searchKey;
        cnt = 1;
    }

    skiplist_node(K searchKey, V val)
    {
        for (int i = 1; i <= MAXLEVEL; i++)
        {
            forwards[i] = NULL;
        }
        key[0] = searchKey;
        value[0] = val;
        cnt = 1;
    }

    virtual ~skiplist_node()
    {
    }

    void insert(K k, V v)
    {
        for (int i = 0; i < cnt; i++)
        {
            if (key[i] < k)
                continue;

            // shift to right
            for (int j = cnt - 1; j >= i; j--)
            {
                key[j + 1] = key[j];
                value[j + 1] = value[j];
            }
            // insert to the right position
            key[i] = k;
            value[i] = v;
            cnt++;
            return;
        }
        key[cnt] = k;
        value[cnt] = v;
        cnt++;
        return;
    }
};

///////////////////////////////////////////////////////////////////////////////

template <class K, class V, int MAXLEVEL = 16>
class skiplist
{
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
        NodeType *currNode = m_pHeader->forwards[1];
        while (currNode != m_pTail)
        {
            NodeType *tempNode = currNode;
            currNode = currNode->forwards[1];
            delete tempNode;
        }
        delete m_pHeader;
        delete m_pTail;
    }

    void insert(K searchKey, V newValue)
    {
        skiplist_node<K, V, MAXLEVEL> *update[MAXLEVEL];
        NodeType *currNode = m_pHeader;
        for (int level = max_curr_level; level >= 1; level--)
        {
            while (currNode->forwards[level]->key[0] <= searchKey)
            {
                currNode = currNode->forwards[level];
            }
            update[level] = currNode;
        }

        //currNode = currNode->forwards[1];

        if (currNode->cnt < NPAIRS)
        {
            //  insert
            currNode->insert(searchKey, newValue);
        }
        else
        { // split
            int newlevel = randomLevel();
            if (newlevel > max_curr_level)
            {
                for (int level = max_curr_level + 1; level <= newlevel; level++)
                {
                    update[level] = m_pHeader;
                }
                max_curr_level = newlevel;
            }

            //currNode = new NodeType(searchKey,newValue);
            NodeType *newNode = new NodeType();
            int mid = currNode->cnt / 2;
            for (int i = mid; i < currNode->cnt; i++)
            {
                newNode->insert(currNode->key[i], currNode->value[i]);
            }
            currNode->cnt = mid;
            if (newNode->key[0] < searchKey)
            {
                newNode->insert(searchKey, newValue);
            }
            else
            {
                currNode->insert(searchKey, newValue);
            }

            for (int lv = 1; lv <= max_curr_level; lv++)
            {
                newNode->forwards[lv] = update[lv]->forwards[lv];
                update[lv]->forwards[lv] = newNode; // make previous node point to new node
            }
        }
    }

    void erase(K searchKey)
    {
        /*
        skiplist_node<K,V,MAXLEVEL>* update[MAXLEVEL];
        NodeType* currNode = m_pHeader;
        for(int level=max_curr_level; level >=1; level--) {
            while ( currNode->forwards[level]->key < searchKey ) {
                currNode = currNode->forwards[level];
            }
            update[level] = currNode;
        }
        currNode = currNode->forwards[1];
        if ( currNode->key == searchKey ) {
            for ( int lv = 1; lv <= max_curr_level; lv++ ) {
                if ( update[lv]->forwards[lv] != currNode ) {
                    break;
                }
                update[lv]->forwards[lv] = currNode->forwards[lv];
            }
            delete currNode;
            // update the max level
            while ( max_curr_level > 1 && m_pHeader->forwards[max_curr_level] == NULL ) {
                max_curr_level--;
            }
        }
	*/
    }

    //const NodeType* find(K searchKey)
    V find(K searchKey)
    {
        NodeType *currNode = m_pHeader;
        for (int level = max_curr_level; level >= 1; level--)
        {
            while (currNode->forwards[level]->key[0] <= searchKey)
            {
                currNode = currNode->forwards[level];
            }
        }
        // currNode = currNode->forwards[1];

        for (int i = 0; i < currNode->cnt; i++)
        {
            if (currNode->key[i] == searchKey)
            {
                return currNode->value[i];
            }
        }

        //return NULL;
        return -1;
    }

    bool empty() const
    {
        return (m_pHeader->forwards[1] == m_pTail);
    }

    std::string printList()
    {
        int i = 0;
        std::stringstream sstr;
        NodeType *currNode = m_pHeader; //->forwards[1];
        while (currNode != m_pTail)
        {
            sstr << "(";
            for (int i = 0; i < currNode->cnt; i++)
            {
                sstr << currNode->key[i] << ",";
            }
            sstr << ")";
            currNode = currNode->forwards[1];
            i++;
            if (i > 200)
                break;
        }
        return sstr.str();
    }

    const int max_level;

protected:
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
};

///////////////////////////////////////////////////////////////////////////////
