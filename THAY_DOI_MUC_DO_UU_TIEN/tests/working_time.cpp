#include "../src/THAY_DOI_TU_DONG/WorkingTime.h"

#include <cassert>
#include <ctime>
#include <string>

namespace {
time_t localTimestamp(int year, int month, int day, int hour, int minute)
{
    tm value{};
    value.tm_year = year - 1900;
    value.tm_mon = month - 1;
    value.tm_mday = day;
    value.tm_hour = hour;
    value.tm_min = minute;
    value.tm_isdst = -1;
    return std::mktime(&value);
}

void expectTime(time_t actual, int year, int month, int day, int hour, int minute)
{
    tm value{};
#ifdef _WIN32
    assert(localtime_s(&value, &actual) == 0);
#else
    assert(localtime_r(&actual, &value) != nullptr);
#endif
    assert(value.tm_year + 1900 == year);
    assert(value.tm_mon + 1 == month);
    assert(value.tm_mday == day);
    assert(value.tm_hour == hour);
    assert(value.tm_min == minute);
}
}

int main()
{
    const time_t mondayMorning = localTimestamp(2026, 10, 5, 8, 0);
    expectTime(calculateNextBoostTime(mondayMorning), 2026, 10, 5, 9, 30);
    expectTime(calculateNextBoostTime(localTimestamp(2026, 10, 5, 10, 30)),
               2026, 10, 5, 13, 30);
    expectTime(calculateNextBoostTime(localTimestamp(2026, 10, 5, 11, 0)),
               2026, 10, 5, 14, 0);
    expectTime(calculateNextBoostTime(localTimestamp(2026, 10, 5, 16, 0)),
               2026, 10, 6, 8, 0);
    expectTime(calculateNextBoostTime(localTimestamp(2026, 10, 5, 18, 0)),
               2026, 10, 6, 9, 0);
    expectTime(calculateNextBoostTime(localTimestamp(2026, 10, 10, 8, 0)),
               2026, 10, 10, 9, 30);
    expectTime(calculateNextBoostTime(localTimestamp(2026, 10, 5, 23, 0), true),
               2026, 10, 6, 0, 30);

    assert(isWorkingTime(localTimestamp(2026, 10, 5, 7, 30)));
    assert(!isWorkingTime(localTimestamp(2026, 10, 5, 11, 30)));
    assert(isWorkingTime(localTimestamp(2026, 10, 5, 16, 59)));
    assert(!isWorkingTime(localTimestamp(2026, 10, 5, 17, 0)));
}
