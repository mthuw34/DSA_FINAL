#ifndef AUTO_PRIORITY_ITEM_H
#define AUTO_PRIORITY_ITEM_H

#include <string>

using namespace std;

struct AutoPriorityItem
{
    int checkinId;
    int patientId;

    string checkinTime;

    int currentPriority;

    // Thoi diem benh nhan da cho du 1 gio
    long long dueTime;
};

#endif