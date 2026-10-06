#ifndef AUTO_PRIORITY_H
#define AUTO_PRIORITY_H

#include <ctime>
#include <sqlite3.h>

// Bản ghi trong heap ưu tiên tự động.
struct AutoPriorityItem
{
    int checkinId;
    time_t nextBoostTime;
};

// Heap ưu tiên tự động, sắp xếp theo thời gian tiếp theo để tăng mức ưu tiên.
class AutoPriorityHeap
{
    //Các dữ liệu và phương thức riêng tư của heap.
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

// Các hàm hỗ trợ xử lý mức ưu tiên tự động.
void loadPatients(sqlite3* db, AutoPriorityHeap& heap);
void processAuto(sqlite3* db, AutoPriorityHeap& heap);

#endif