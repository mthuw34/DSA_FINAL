#include "AutoPriority.h"

//Khởi tạo
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

    heap =
        new AutoPriorityItem[capacity];
}

//Giải phóng bộ nhớ
AutoPriorityHeap::~AutoPriorityHeap()
{
    delete[] heap;
}

// VI TRI CHA VA CON
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

// SO SANH HAI PHAN TU
bool AutoPriorityHeap::higherPriority(
    const AutoPriorityItem& a,
    const AutoPriorityItem& b
)
{
    //Còn ít thời gian thì đến hạn khám sớm hơn 
    if (
        a.remainingSeconds
        !=
        b.remainingSeconds
    )
    {
        return
            a.remainingSeconds
            <
            b.remainingSeconds;
    }

    //Bằng nhau -> checkin_id nhỏ hơn đứng trước
    return
        a.checkinId
        <
        b.checkinId;
}

//Đổi chỗ 
void AutoPriorityHeap::swapItem(
    AutoPriorityItem& a,
    AutoPriorityItem& b
)
{
    AutoPriorityItem temp = a;

    a = b;

    b = temp;
}

//Mở rộng mảng động

void AutoPriorityHeap::resize()
{
    int newCapacity =
        capacity * 2;

    AutoPriorityItem* newHeap =
        new AutoPriorityItem[newCapacity];


    for (
        int i = 0;
        i < size;
        i++
    )
    {
        newHeap[i] = heap[i];
    }


    delete[] heap;


    heap = newHeap;

    capacity = newCapacity;
}

//Sift up
void AutoPriorityHeap::siftUp(
    int index
)
{
    while (index > 0)
    {
        int p =
            parent(index);


        if (
            higherPriority(
                heap[index],
                heap[p]
            )
        )
        {
            swapItem(
                heap[index],
                heap[p]
            );

            index = p;
        }

        else
        {
            break;
        }
    }
}

//Sift down
void AutoPriorityHeap::siftDown(
    int index
)
{
    while (true)
    {
        int left =
            leftChild(index);

        int right =
            rightChild(index);

        int smallest =
            index;


        if (
            left < size
            &&
            higherPriority(
                heap[left],
                heap[smallest]
            )
        )
        {
            smallest = left;
        }


        if (
            right < size
            &&
            higherPriority(
                heap[right],
                heap[smallest]
            )
        )
        {
            smallest = right;
        }

        if (smallest == index)
        {
            break;
        }

        swapItem(
            heap[index],
            heap[smallest]
        );


        index = smallest;
    }
}

// INSERT
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

//Peek
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

//Extract min
bool AutoPriorityHeap::extractMin(
    AutoPriorityItem& item
)
{
    if (size == 0)
    {
        return false;
    }

    item = heap[0];

    heap[0] =
        heap[size - 1];

    size--;


    if (size > 0)
    {
        siftDown(0);
    }

    return true;
}


//Kiểm tra rộng
bool AutoPriorityHeap::isEmpty() const
{
    return size == 0;
}

//Số lượng phần tử 
int AutoPriorityHeap::getSize() const
{
    return size;
}