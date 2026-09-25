#include "AutoPriorityHeap.h"

using namespace std;


// ====================================
// KHOI TAO HEAP
// ====================================

AutoPriorityHeap::AutoPriorityHeap(
    int initialCapacity
)
{
    if (initialCapacity <= 0)
    {
        initialCapacity = 10;
    }

    size = 0;

    capacity = initialCapacity;

    heap = new AutoPriorityItem[capacity];
}


// ====================================
// GIAI PHONG BO NHO
// ====================================

AutoPriorityHeap::~AutoPriorityHeap()
{
    delete[] heap;
}


// ====================================
// VI TRI CHA VA CAC CON
// ====================================

int AutoPriorityHeap::parent(int i)
{
    return (i - 1) / 2;
}

int AutoPriorityHeap::leftChild(int i)
{
    return 2 * i + 1;
}

int AutoPriorityHeap::rightChild(int i)
{
    return 2 * i + 2;
}


// ====================================
// SO SANH HAI BENH NHAN
// ====================================

bool AutoPriorityHeap::isEarlier(
    const AutoPriorityItem& a,
    const AutoPriorityItem& b
)
{
    // Ai con it thoi gian cho hon
    // thi duoc dua len truoc

    if (a.remainingSeconds != b.remainingSeconds)
    {
        return a.remainingSeconds < b.remainingSeconds;
    }

    // Neu bang nhau, uu tien checkin_id nho hon

    return a.checkinId < b.checkinId;
}


// ====================================
// DOI CHO HAI PHAN TU
// ====================================

void AutoPriorityHeap::swapItem(
    AutoPriorityItem& a,
    AutoPriorityItem& b
)
{
    AutoPriorityItem temp = a;

    a = b;

    b = temp;
}


// ====================================
// MO RONG MANG DONG
// ====================================

void AutoPriorityHeap::resize()
{
    int newCapacity = capacity * 2;

    AutoPriorityItem* newHeap =
        new AutoPriorityItem[newCapacity];

    for (int i = 0; i < size; i++)
    {
        newHeap[i] = heap[i];
    }

    delete[] heap;

    heap = newHeap;

    capacity = newCapacity;
}


// ====================================
// SIFT UP
// ====================================

void AutoPriorityHeap::siftUp(int i)
{
    while (i > 0)
    {
        int p = parent(i);

        if (isEarlier(heap[i], heap[p]))
        {
            swapItem(heap[i], heap[p]);

            i = p;
        }
        else
        {
            break;
        }
    }
}


// ====================================
// SIFT DOWN
// ====================================

void AutoPriorityHeap::siftDown(int i)
{
    while (true)
    {
        int left = leftChild(i);

        int right = rightChild(i);

        int smallest = i;

        // Kiem tra con trai

        if (
            left < size &&
            isEarlier(heap[left], heap[smallest])
        )
        {
            smallest = left;
        }

        // Kiem tra con phai

        if (
            right < size &&
            isEarlier(heap[right], heap[smallest])
        )
        {
            smallest = right;
        }

        if (smallest == i)
        {
            break;
        }

        swapItem(heap[i], heap[smallest]);

        i = smallest;
    }
}


// ====================================
// THEM MOT BENH NHAN
// ====================================

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


// ====================================
// XEM BENH NHAN DEN HAN SOM NHAT
// ====================================

bool AutoPriorityHeap::peek(
    AutoPriorityItem& item
) const
{
    if (size == 0)
    {
        return false;
    }

    item = heap[0];

    return true;
}


// ====================================
// LAY BENH NHAN DEN HAN SOM NHAT
// ====================================

bool AutoPriorityHeap::extractMin(
    AutoPriorityItem& item
)
{
    if (size == 0)
    {
        return false;
    }

    // Luu phan tu o root

    item = heap[0];

    // Dua phan tu cuoi len root

    heap[0] = heap[size - 1];

    size--;

    // Khoi phuc tinh chat Heap

    if (size > 0)
    {
        siftDown(0);
    }

    return true;
}


// ====================================
// KIEM TRA HEAP RONG
// ====================================

bool AutoPriorityHeap::isEmpty() const
{
    return size == 0;
}


// ====================================
// SO LUONG PHAN TU
// ====================================

int AutoPriorityHeap::getSize() const
{
    return size;
}