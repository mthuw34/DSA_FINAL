#ifndef WORKING_TIME_H
#define WORKING_TIME_H

#include <ctime>

// Kiem tra thoi diem co nam trong gio lam viec
bool isWorkingTime(time_t timestamp);

// Tinh tong so giay cho trong gio lam viec
long long calculateWorkingSeconds(
    time_t checkinTime,
    time_t currentTime
);

#endif