#include "AutoPriority.h"

// dùng cho các hàm xử lý mức ưu tiên tự động.
AutoPriorityHeap::AutoPriorityHeap(int initialCapacity) 
{
    if (initialCapacity <= 0)
    {
        initialCapacity = 10;
    }

    size = 0;
    capacity = initialCapacity;
    heap = new AutoPriorityItem[capacity];
}

// Hủy bộ nhớ heap.
AutoPriorityHeap::~AutoPriorityHeap()
{
    delete[] heap;
}

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

// Mốc thời gian tiếp theo để tăng mức ưu tiên thấp hơn được coi là ưu tiên cao hơn.
bool AutoPriorityHeap::higherPriority(const AutoPriorityItem& a,const AutoPriorityItem& b)
{
    if (a.nextBoostTime != b.nextBoostTime)
    {
        return a.nextBoostTime < b.nextBoostTime;
    }

    return a.checkinId < b.checkinId;
}

// Hoán đổi hai phần tử trong heap.
void AutoPriorityHeap::swapItem(AutoPriorityItem& a, AutoPriorityItem& b)
{
    AutoPriorityItem temp = a;
    a = b;
    b = temp;
}

// Resize heap khi cần thiết.
void AutoPriorityHeap::resize()
{
    int newCapacity = capacity * 2;
    AutoPriorityItem* newHeap = new AutoPriorityItem[newCapacity];

    for (int i = 0; i < size; i++)
    {
        newHeap[i] = heap[i];
    }

    delete[] heap;
    heap = newHeap;
    capacity = newCapacity;
}

void AutoPriorityHeap::siftUp(int index)
{
    while (index > 0)
    {
        int p = parent(index);
        if (higherPriority(heap[index], heap[p]))
        {
            swapItem(heap[index], heap[p]);
            index = p;
        }
        else break;
    }
}

void AutoPriorityHeap::siftDown(int index)
{
    //Lặp lại cho đến khi phần tử được đặt đúng vị trí.
    while (true)
    {
        int left = leftChild(index);
        int right = rightChild(index);
        int smallest = index;

        if (
            left < size &&
            higherPriority(heap[left], heap[smallest])
        )
        {
            smallest = left;
        }

        if (right < size &&
            higherPriority(heap[right], heap[smallest]))
        {
            smallest = right;
        }

        if (smallest == index) break;

        swapItem(heap[index], heap[smallest]);
        index = smallest;
    }
}

// Thêm một phần tử vào heap.
void AutoPriorityHeap::insert(const AutoPriorityItem& item)
{
    if (size == capacity)
    {
        resize();
    }

    heap[size] = item;
    siftUp(size);
    size++;
}

// Lấy phần tử có ưu tiên cao nhất mà không xóa nó khỏi heap.
bool AutoPriorityHeap::peek(AutoPriorityItem& item) const
{
    if (size == 0)
    {
        return false;
    }

    item = heap[0];
    return true;
}

// Lấy phần tử có ưu tiên cao nhất và xóa nó khỏi heap.
bool AutoPriorityHeap::extractMin(AutoPriorityItem& item)
{
    if (size == 0)
    {
        return false;
    }

    item = heap[0];
    heap[0] = heap[size - 1];
    size--;

    if (size > 0)
    {
        siftDown(0);
    }

    return true;
}

// Kiểm tra xem heap có rỗng không.
bool AutoPriorityHeap::isEmpty() const
{
    return size == 0;
}

int AutoPriorityHeap::getSize() const
{
    return size;
}