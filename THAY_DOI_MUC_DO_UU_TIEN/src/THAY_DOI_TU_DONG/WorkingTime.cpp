#include "WorkingTime.h"
using namespace std;


// 07:30
const int SANG_BAT_DAU = 7 * 60 + 30;

// 11:30
const int SANG_KET_THUC = 11 * 60 + 30;

// 13:00
const int CHIEU_BAT_DAU = 13 * 60;

// 16:30
const int CHIEU_KET_THUC = 16 * 60 + 30;


bool isWorkingTime(time_t timestamp)
{
    tm info = *localtime(&timestamp);
    int minutes = info.tm_hour * 60 + info.tm_min;

    bool morning =
        minutes >= SANG_BAT_DAU &&
        minutes <= SANG_KET_THUC;

    bool afternoon =
        minutes >= CHIEU_BAT_DAU &&
        minutes <= CHIEU_KET_THUC;

    return morning || afternoon;
}


time_t calculateNextBoostTime(time_t startTime)
{
    const int BOOST_SECONDS = 90 * 60;

    int remaining = BOOST_SECONDS;
    time_t current = startTime;

    while (remaining > 0)
    {
        tm info = *localtime(&current);

        int minutes = info.tm_hour * 60 + info.tm_min;

        // Ca sáng
        if (
            minutes >= SANG_BAT_DAU &&
            minutes < SANG_KET_THUC
        )
        {
            tm endMorning = info;

            endMorning.tm_hour = 11;
            endMorning.tm_min = 30;
            endMorning.tm_sec = 0;

            time_t endTime = mktime(&endMorning);

            int available = static_cast<int>(
                    endTime - current
                );

            if (remaining <= available)
            {
                return current + remaining;
            }

            remaining -= available;
            current = endTime;
        }

        // Nghỉ trưa
        else if (
            minutes >= SANG_KET_THUC &&
            minutes < CHIEU_BAT_DAU
        )
        {
            tm afternoon = info;

            afternoon.tm_hour = 13;
            afternoon.tm_min = 0;
            afternoon.tm_sec = 0;

            current = mktime(&afternoon);
        }

        // Ca chiều
        else if (
            minutes >= CHIEU_BAT_DAU &&
            minutes < CHIEU_KET_THUC
        )
        {
            tm endAfternoon = info;

            endAfternoon.tm_hour = 16;
            endAfternoon.tm_min = 30;
            endAfternoon.tm_sec = 0;

            time_t endTime = mktime(&endAfternoon);

            int available = static_cast<int>(endTime - current);

            if (remaining <= available)
            {
                return current + remaining;
            }

            remaining -= available;
            current = endTime;
        }


        // Ngoài giờ làm việc
        else
        {
            tm next = info;

            if (minutes < SANG_BAT_DAU)
            {
                next.tm_hour = 7;
                next.tm_min = 30;
                next.tm_sec = 0;
            }
            else
            {
                next.tm_mday++;

                next.tm_hour = 7;
                next.tm_min = 30;
                next.tm_sec = 0;
            }
            current = mktime(&next);
        }
    }

    return current;
}