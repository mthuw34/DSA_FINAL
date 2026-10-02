#ifndef AUTO_PRIORITY_H
#define AUTO_PRIORITY_H

// Thông tin của 1 lượt checkin 
struct AutoPriorityItem
{
    int checkinId;
    int currentPriority;
    
    // Tổng thời gian chờ hợp lệ 
    long long waitingSeconds;

    // Mốc 90 phút tiếp theo
    int nextPeriod;

    // Số giây còn lại để đến mốc tiếp theo
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