#include "AutoPriorityHeap.h"

using namespace std;


// ================================
// CONSTRUCTOR
// ================================

AutoPriorityHeap::AutoPriorityHeap(
    int initialCapacity
)
{
    capacity = initialCapacity;

    size = 0;

    heap =
        new AutoPriorityItem[capacity];
}


// ================================
// DESTRUCTOR
// ================================

AutoPriorityHeap::~AutoPriorityHeap()
{
    delete[] heap;
}


// ================================
// VI TRI CHA
// ================================

int AutoPriorityHeap::parent(int i)
{
    return (i - 1) / 2;
}


// ================================
// CON TRAI
// ================================

int AutoPriorityHeap::leftChild(int i)
{
    return 2 * i + 1;
}


// ================================
// CON PHAI
// ================================

int AutoPriorityHeap::rightChild(int i)
{
    return 2 * i + 2;
}


// ================================
// DOI CHO 2 PHAN TU
// ================================

void AutoPriorityHeap::swapItem(
    AutoPriorityItem& a,
    AutoPriorityItem& b
)
{
    AutoPriorityItem temp = a;

    a = b;

    b = temp;
}


// ================================
// MO RONG MANG
// ================================

void AutoPriorityHeap::resize()
{
    capacity *= 2;

    AutoPriorityItem* newHeap =
        new AutoPriorityItem[capacity];

    for (int i = 0; i < size; i++)
    {
        newHeap[i] = heap[i];
    }

    delete[] heap;

    heap = newHeap;
}


// ================================
// SIFT UP
// ================================

void AutoPriorityHeap::siftUp(int i)
{
    while (i > 0)
    {
        int p = parent(i);

        if (
            heap[i].dueTime
            <
            heap[p].dueTime
        )
        {
            swapItem(
                heap[i],
                heap[p]
            );

            i = p;
        }
        else
        {
            break;
        }
    }
}


// ================================
// SIFT DOWN
// ================================

void AutoPriorityHeap::siftDown(int i)
{
    while (true)
    {
        int left =
            leftChild(i);

        int right =
            rightChild(i);

        int smallest = i;

        if (
            left < size &&
            heap[left].dueTime
                <
            heap[smallest].dueTime
        )
        {
            smallest = left;
        }

        if (
            right < size &&
            heap[right].dueTime
                <
            heap[smallest].dueTime
        )
        {
            smallest = right;
        }

        if (smallest == i)
        {
            break;
        }

        swapItem(
            heap[i],
            heap[smallest]
        );

        i = smallest;
    }
}


// ================================
// INSERT
// ================================

void AutoPriorityHeap::insert(
    const AutoPriorityItem& item
)
{
    if (size == capacity)
    {
        resize();
    }

    heap[size] = item;

    int index = size;

    size++;

    siftUp(index);
}


// ================================
// PEEK
// ================================

AutoPriorityItem AutoPriorityHeap::peek()
{
    if (size == 0)
    {
        return AutoPriorityItem();
    }

    return heap[0];
}


// ================================
// EXTRACT MIN
// ================================

AutoPriorityItem AutoPriorityHeap::extractMin()
{
    if (size == 0)
    {
        return AutoPriorityItem();
    }

    AutoPriorityItem result =
        heap[0];

    heap[0] =
        heap[size - 1];

    size--;

    if (size > 0)
    {
        siftDown(0);
    }

    return result;
}


// ================================
// KIEM TRA RONG
// ================================

bool AutoPriorityHeap::isEmpty()
{
    return size == 0;
}


// ================================
// SO LUONG PHAN TU
// ================================

int AutoPriorityHeap::getSize()
{
    return size;
}