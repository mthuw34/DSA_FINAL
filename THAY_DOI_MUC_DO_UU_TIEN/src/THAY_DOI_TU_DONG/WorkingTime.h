#ifndef WORKING_TIME_H
#define WORKING_TIME_H

#include <ctime>

bool isWorkingTime(
    time_t timestamp
);

time_t calculateNextBoostTime(
    time_t startTime,
    bool emergencyDepartment = false
);

#endif