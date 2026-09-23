#ifndef AUTO_PRIORITY_HEAP_H
#define AUTO_PRIORITY_HEAP_H

#include "AutoPriorityItem.h"

class AutoPriorityHeap
{
private:
    AutoPriorityItem* heap;

    int size;
    int capacity;

    int parent(int i);
    int leftChild(int i);
    int rightChild(int i);

    void swapItem(
        AutoPriorityItem& a,
        AutoPriorityItem& b
    );

    void resize();

    void siftUp(int i);
    void siftDown(int i);

public:
    AutoPriorityHeap(int initialCapacity = 10);

    ~AutoPriorityHeap();

    bool isEmpty();

    int getSize();

    void insert(
        const AutoPriorityItem& item
    );

    AutoPriorityItem peek();

    AutoPriorityItem extractMin();
};

#endif