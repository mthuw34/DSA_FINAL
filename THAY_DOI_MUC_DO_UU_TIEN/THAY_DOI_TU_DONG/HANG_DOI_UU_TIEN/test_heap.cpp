#include <iostream>

#include "AutoPriorityHeap.h"

using namespace std;

int main()
{
    AutoPriorityHeap heap;

    // Benh nhan 1
    // Con 30 phut de den moc tiep theo

    AutoPriorityItem a = {
        1,
        5400,
        2,
        1800
    };


    // Benh nhan 2
    // Con 10 phut de den moc tiep theo

    AutoPriorityItem b = {
        2,
        3000,
        1,
        600
    };


    // Benh nhan 3
    // Con 20 phut de den moc tiep theo

    AutoPriorityItem c = {
        3,
        6000,
        2,
        1200
    };


    // Them vao Heap

    heap.insert(a);
    heap.insert(b);
    heap.insert(c);


    cout << "So benh nhan: "
         << heap.getSize()
         << '\n';


    // Kiem tra root

    AutoPriorityItem item;
    
    // Lay lan luot cac phan tu

    cout << "\nThu tu xu ly:\n";

    while (heap.extractMin(item))
    {
        cout
            << "Checkin ID: "
            << item.checkinId

            << " | Con phai cho: "
            << item.remainingSeconds / 60

            << " phut\n";
    }

    return 0;
}   