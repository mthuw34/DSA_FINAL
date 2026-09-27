#ifndef AUTO_PRIORITY_H
#define AUTO_PRIORITY_H


// Thong tin cua mot luot check-in
struct AutoPriorityItem
{
    int checkinId;

    // Tong thoi gian cho hop le
    long long waitingSeconds;

    // Moc 90 phut tiep theo
    int nextPeriod;

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

    AutoPriorityHeap(
        int initialCapacity = 10
    );

    ~AutoPriorityHeap();


    void insert(
        const AutoPriorityItem& item
    );


    bool peek(
        AutoPriorityItem& item
    ) const;


    bool extractMin(
        AutoPriorityItem& item
    );


    bool isEmpty() const;

    int getSize() const;
};


#endif