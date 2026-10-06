#include "WorkingTime.h"

#include <ctime>

namespace {
constexpr int MORNING_START = 7 * 60 + 30;
constexpr int MORNING_END = 11 * 60 + 30;
constexpr int AFTERNOON_START = 13 * 60;
constexpr int AFTERNOON_END = 17 * 60;
constexpr int BOOST_SECONDS = 90 * 60;

// Chuyển đổi thời gian từ time_t sang tm theo múi giờ địa phương.
bool localTime(time_t timestamp, tm& result)
{
#ifdef _WIN32
    return localtime_s(&result, &timestamp) == 0;
#else
    return localtime_r(&timestamp, &result) != nullptr;
#endif
}

// Tạo time_t từ tm và phút trong ngày.
time_t atMinuteOfDay(tm day, int minuteOfDay)
{
    day.tm_hour = minuteOfDay / 60;
    day.tm_min = minuteOfDay % 60;
    day.tm_sec = 0;
    day.tm_isdst = -1;
    return mktime(&day);
}
}

// Kiểm tra xem thời gian có nằm trong khung giờ làm việc hay không.
bool isWorkingTime(time_t timestamp)
{
    tm local{};
    if (!localTime(timestamp, local)) return false;
    const int minuteOfDay = local.tm_hour * 60 + local.tm_min;
    return (minuteOfDay >= MORNING_START && minuteOfDay < MORNING_END) ||
        (minuteOfDay >= AFTERNOON_START && minuteOfDay < AFTERNOON_END);
}

// Tính toán thời gian tiếp theo khi mức ưu tiên được tăng lên.
time_t calculateNextBoostTime(time_t startTime, bool emergencyDepartment)
{
    if (emergencyDepartment) return startTime + BOOST_SECONDS;

    int remaining = BOOST_SECONDS;
    time_t current = startTime;
    
    // Nếu thời gian bắt đầu không nằm trong khung giờ làm việc, di chuyển đến khung giờ tiếp theo.
    while (remaining > 0) {
        tm local{};
        if (!localTime(current, local)) return static_cast<time_t>(-1);
        const int minuteOfDay = local.tm_hour * 60 + local.tm_min;

        int windowEnd = 0;
        if (minuteOfDay < MORNING_START) {
            current = atMinuteOfDay(local, MORNING_START);
            continue;
        }
        if (minuteOfDay < MORNING_END) {
            windowEnd = MORNING_END;
        } else if (minuteOfDay < AFTERNOON_START) {
            current = atMinuteOfDay(local, AFTERNOON_START);
            continue;
        } else if (minuteOfDay < AFTERNOON_END) {
            windowEnd = AFTERNOON_END;
        } else {
            ++local.tm_mday;
            current = atMinuteOfDay(local, MORNING_START);
            continue;
        }

        // Tính toán thời gian còn lại trong khung giờ làm việc hiện tại.
        const time_t endTime = atMinuteOfDay(local, windowEnd);
        const int available = static_cast<int>(endTime - current);
        if (remaining <= available) return current + remaining;
        remaining -= available;
        current = endTime;
    }
    return current;
}
