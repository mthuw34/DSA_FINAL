#ifndef WORKING_TIME_H
#define WORKING_TIME_H

#include <ctime>


bool isWorkingTime(
    time_t timestamp
);


long long calculateWorkingSeconds(
    time_t checkinTime,
    time_t currentTime
);


#endif