#ifndef AUTO_PRIORITY_H
#define AUTO_PRIORITY_H

#include <ctime>

struct AutoPriorityItem
{
    int checkinId;
    time_t nextBoostTime;
};

class AutoPriorityHeap
{
private:
    AutoPriorityItem* heap;
    int size;
    int capacity;

    int parent(int i);
    int leftChild(int i);
    int rightChild(int i);

    bool higherPriority(
        const AutoPriorityItem& a,
        const AutoPriorityItem& b
    );

    void swapItem(
        AutoPriorityItem& a,
        AutoPriorityItem& b
    );

    void resize();
    void siftUp(int index);
    void siftDown(int index);

public:
    AutoPriorityHeap(int initialCapacity = 10);
    ~AutoPriorityHeap();

    void insert(const AutoPriorityItem& item);

    bool peek(AutoPriorityItem& item) const;

    bool extractMin(AutoPriorityItem& item);

    bool isEmpty() const;

    int getSize() const;
};

#endif