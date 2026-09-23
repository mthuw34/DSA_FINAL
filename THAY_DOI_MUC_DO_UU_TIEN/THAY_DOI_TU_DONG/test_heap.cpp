#include <iostream>

#include "AutoPriorityHeap.h"

using namespace std;

int main()
{
    AutoPriorityHeap heap;

    // Benh nhan 1
    AutoPriorityItem a;

    a.checkinId = 1;
    a.patientId = 101;
    a.checkinTime = "2026-09-23 08:00:00";
    a.currentPriority = 4;
    a.dueTime = 300;


    // Benh nhan 2
    AutoPriorityItem b;

    b.checkinId = 2;
    b.patientId = 102;
    b.checkinTime = "2026-09-23 07:00:00";
    b.currentPriority = 3;
    b.dueTime = 100;


    // Benh nhan 3
    AutoPriorityItem c;

    c.checkinId = 3;
    c.patientId = 103;
    c.checkinTime = "2026-09-23 07:30:00";
    c.currentPriority = 5;
    c.dueTime = 200;


    // =============================
    // INSERT VAO MIN-HEAP
    // =============================

    heap.insert(a);
    heap.insert(b);
    heap.insert(c);


    cout << "So phan tu trong heap: "
         << heap.getSize()
         << '\n';


    // =============================
    // KIEM TRA ROOT
    // =============================

    AutoPriorityItem root =
        heap.peek();

    cout << "\nPhan tu o root:\n";

    cout << "Checkin ID: "
         << root.checkinId
         << '\n';

    cout << "Due time: "
         << root.dueTime
         << '\n';


    // =============================
    // EXTRACT LAN LUOT
    // =============================

    cout << "\nThu tu extract:\n";

    while (!heap.isEmpty())
    {
        AutoPriorityItem item =
            heap.extractMin();

        cout
            << "Checkin ID: "
            << item.checkinId

            << " | Due time: "
            << item.dueTime

            << '\n';
    }

    return 0;
}