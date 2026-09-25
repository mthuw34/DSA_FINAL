#ifndef AUTO_PRIORITY_HEAP_H
#define AUTO_PRIORITY_HEAP_H

// Thong tin can thiet de theo doi
// moc tang uu tien cua mot luot check-in
struct AutoPriorityItem
{
    int checkinId;

    // Tong thoi gian cho hop le
    long long waitingSeconds;

    // Moc 60 phut tiep theo
    int nextHour;

    // So giay con lai de den moc tiep theo
    long long remainingSeconds;
};


// Binary Min-Heap tu cai dat
class AutoPriorityHeap
{
private:

    AutoPriorityItem* heap;

    int size;
    int capacity;

    // Tim vi tri cha va cac con
    int parent(int i);
    int leftChild(int i);
    int rightChild(int i);

    // So sanh hai phan tu
    bool isEarlier(
        const AutoPriorityItem& a,
        const AutoPriorityItem& b
    );

    // Doi cho
    void swapItem(
        AutoPriorityItem& a,
        AutoPriorityItem& b
    );

    // Mo rong mang dong
    void resize();

    // Hai thuat toan duy tri Heap
    void siftUp(int i);
    void siftDown(int i);

public:

    AutoPriorityHeap(int initialCapacity = 10);

    ~AutoPriorityHeap();

    // Khong cho phep sao chep mang
    AutoPriorityHeap(const AutoPriorityHeap&) = delete;

    AutoPriorityHeap& operator=(
        const AutoPriorityHeap&
    ) = delete;

    void insert(const AutoPriorityItem& item);

    bool peek(AutoPriorityItem& item) const;

    bool extractMin(AutoPriorityItem& item);

    bool isEmpty() const;

    int getSize() const;
};

#endif