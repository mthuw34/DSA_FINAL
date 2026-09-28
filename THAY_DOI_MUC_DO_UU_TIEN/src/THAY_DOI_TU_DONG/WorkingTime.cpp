#include "WorkingTime.h"

#include <algorithm>

using namespace std;


// 07:30
const int SANG_BAT_DAU =
    7 * 60 + 30;


// 11:30
const int SANG_KET_THUC =
    11 * 60 + 30;


// 13:00
const int CHIEU_BAT_DAU =
    13 * 60;


// 16:30
const int CHIEU_KET_THUC =
    16 * 60 + 30;


// ========================================
// KIEM TRA CO DANG TRONG GIO LAM KHONG
// ========================================

bool isWorkingTime(
    time_t timestamp
)
{
    tm timeInfo =
        *localtime(&timestamp);


    int minutes =
        timeInfo.tm_hour * 60
        +
        timeInfo.tm_min;


    bool morning =
        minutes >= SANG_BAT_DAU
        &&
        minutes < SANG_KET_THUC;


    bool afternoon =
        minutes >= CHIEU_BAT_DAU
        &&
        minutes < CHIEU_KET_THUC;


    return
        morning
        ||
        afternoon;
}


// ========================================
// TINH TONG THOI GIAN CHO HOP LE
// ========================================

long long calculateWorkingSeconds(
    time_t checkinTime,
    time_t currentTime
)
{
    if (currentTime <= checkinTime)
    {
        return 0;
    }


    long long totalSeconds = 0;


    tm date =
        *localtime(&checkinTime);


    date.tm_hour = 0;
    date.tm_min = 0;
    date.tm_sec = 0;
    date.tm_isdst = -1;


    time_t dayStart =
        mktime(&date);


    while (dayStart < currentTime)
    {
        tm day =
            *localtime(&dayStart);


        // -----------------------------
        // 07:30
        // -----------------------------

        tm morningStartTm = day;

        morningStartTm.tm_hour = 7;
        morningStartTm.tm_min = 30;
        morningStartTm.tm_sec = 0;
        morningStartTm.tm_isdst = -1;


        time_t morningStart =
            mktime(&morningStartTm);


        // -----------------------------
        // 11:30
        // -----------------------------

        tm morningEndTm = day;

        morningEndTm.tm_hour = 11;
        morningEndTm.tm_min = 30;
        morningEndTm.tm_sec = 0;
        morningEndTm.tm_isdst = -1;


        time_t morningEnd =
            mktime(&morningEndTm);


        // -----------------------------
        // 13:00
        // -----------------------------

        tm afternoonStartTm = day;

        afternoonStartTm.tm_hour = 13;
        afternoonStartTm.tm_min = 0;
        afternoonStartTm.tm_sec = 0;
        afternoonStartTm.tm_isdst = -1;


        time_t afternoonStart =
            mktime(&afternoonStartTm);


        // -----------------------------
        // 16:30
        // -----------------------------

        tm afternoonEndTm = day;

        afternoonEndTm.tm_hour = 16;
        afternoonEndTm.tm_min = 30;
        afternoonEndTm.tm_sec = 0;
        afternoonEndTm.tm_isdst = -1;


        time_t afternoonEnd =
            mktime(&afternoonEndTm);


        // =============================
        // CA SANG
        // =============================

        time_t start =
            max(
                checkinTime,
                morningStart
            );


        time_t end =
            min(
                currentTime,
                morningEnd
            );


        if (end > start)
        {
            totalSeconds +=
                end - start;
        }


        // =============================
        // CA CHIEU
        // =============================

        start =
            max(
                checkinTime,
                afternoonStart
            );


        end =
            min(
                currentTime,
                afternoonEnd
            );


        if (end > start)
        {
            totalSeconds +=
                end - start;
        }


        // Sang ngay tiep theo

        day.tm_mday++;

        day.tm_hour = 0;
        day.tm_min = 0;
        day.tm_sec = 0;
        day.tm_isdst = -1;


        dayStart =
            mktime(&day);
    }


    return totalSeconds;
}