#include "WorkingTime.h"

#include <algorithm>
#include <ctime>

using namespace std;

// Gio lam viec
const int SANG_BAT_DAU = 7 * 60 + 30;
const int SANG_KET_THUC = 11 * 60 + 30;

const int CHIEU_BAT_DAU = 13 * 60;
const int CHIEU_KET_THUC = 16 * 60 + 30;


// ===================================
// HAM KIEM TRA GIO LAM VIEC
// ===================================

bool isWorkingTime(time_t timestamp)
{
    tm timeInfo = *localtime(&timestamp);

    int minutes =
        timeInfo.tm_hour * 60
        + timeInfo.tm_min;

    bool morning =
        minutes >= SANG_BAT_DAU
        && minutes < SANG_KET_THUC;

    bool afternoon =
        minutes >= CHIEU_BAT_DAU
        && minutes < CHIEU_KET_THUC;

    return morning || afternoon;
}


// ===================================
// HAM TINH THOI GIAN CHO HOP LE
// ===================================

long long calculateWorkingSeconds(
    time_t checkinTime,
    time_t currentTime
)
{
    // Thoi gian ket thuc phai lon hon
    // thoi gian check-in
    if (currentTime <= checkinTime)
    {
        return 0;
    }

    long long totalSeconds = 0;

    // Lay ngay check-in
    tm startDate = *localtime(&checkinTime);

    // Dua ve dau ngay
    startDate.tm_hour = 0;
    startDate.tm_min = 0;
    startDate.tm_sec = 0;
    startDate.tm_isdst = -1;

    time_t dayStart = mktime(&startDate);

    // Duyet tung ngay
    while (dayStart < currentTime)
    {
        tm day = *localtime(&dayStart);

        // ==========================
        // THOI GIAN CA SANG
        // ==========================

        tm morningStartInfo = day;

        morningStartInfo.tm_hour = 7;
        morningStartInfo.tm_min = 30;
        morningStartInfo.tm_sec = 0;
        morningStartInfo.tm_isdst = -1;

        time_t morningStart =
            mktime(&morningStartInfo);


        tm morningEndInfo = day;

        morningEndInfo.tm_hour = 11;
        morningEndInfo.tm_min = 30;
        morningEndInfo.tm_sec = 0;
        morningEndInfo.tm_isdst = -1;

        time_t morningEnd =
            mktime(&morningEndInfo);


        // ==========================
        // THOI GIAN CA CHIEU
        // ==========================

        tm afternoonStartInfo = day;

        afternoonStartInfo.tm_hour = 13;
        afternoonStartInfo.tm_min = 0;
        afternoonStartInfo.tm_sec = 0;
        afternoonStartInfo.tm_isdst = -1;

        time_t afternoonStart =
            mktime(&afternoonStartInfo);


        tm afternoonEndInfo = day;

        afternoonEndInfo.tm_hour = 16;
        afternoonEndInfo.tm_min = 30;
        afternoonEndInfo.tm_sec = 0;
        afternoonEndInfo.tm_isdst = -1;

        time_t afternoonEnd =
            mktime(&afternoonEndInfo);


        // ==========================
        // TINH THOI GIAN CA SANG
        // ==========================

        time_t start = max(
            checkinTime,
            morningStart
        );

        time_t end = min(
            currentTime,
            morningEnd
        );

        if (end > start)
        {
            totalSeconds +=
                static_cast<long long>(end - start);
        }


        // ==========================
        // TINH THOI GIAN CA CHIEU
        // ==========================

        start = max(
            checkinTime,
            afternoonStart
        );

        end = min(
            currentTime,
            afternoonEnd
        );

        if (end > start)
        {
            totalSeconds +=
                static_cast<long long>(end - start);
        }


        // ==========================
        // CHUYEN SANG NGAY TIEP THEO
        // ==========================

        day.tm_mday++;

        day.tm_hour = 0;
        day.tm_min = 0;
        day.tm_sec = 0;
        day.tm_isdst = -1;

        dayStart = mktime(&day);
    }

    return totalSeconds;
}