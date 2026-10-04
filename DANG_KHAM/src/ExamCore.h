#pragma once
#include <cstddef>
#include <ctime>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct ExamSession {
    int checkinId = 0, patientId = 0;
    std::string department;
    std::optional<std::string> checkinTime, doctorId, doctorName, startTime, endTime;
    std::optional<std::string> diagnosis, prescription, reminder, updatedAt;
};

struct ExamAssignment {
    ExamSession session;
    std::optional<std::string> plannedEnd;
    std::string status;
};

namespace ExamCore {
// Bảng băm tự cài đặt để đối chiếu check-in giữa lịch phân bác sĩ và lịch sử khám.
class SessionIndex {
    std::vector<std::vector<std::pair<int, std::size_t>>> buckets{4099};
public:
    void put(int id, std::size_t position) {
        auto& entries = buckets[static_cast<unsigned>(id) % buckets.size()];
        for (auto& entry : entries)
            if (entry.first == id) { entry.second = position; return; }
        entries.emplace_back(id, position);
    }
    bool find(int id, std::size_t& position) const {
        for (const auto& entry : buckets[static_cast<unsigned>(id) % buckets.size()])
            if (entry.first == id) { position = entry.second; return true; }
        return false;
    }
};

// Đọc định dạng giờ địa phương YYYY-MM-DD HH:MM:SS do module phân bác sĩ xuất ra.
// Kiểm tra ngày/giờ hợp lệ trước và sau mktime, không để ngày sai bị tự chuẩn hóa.
inline bool parseTime(const std::optional<std::string>& text, std::time_t& output) {
    if (!text || text->size() != 19) return false;
    const auto& s = *text;
    if (s[4] != '-' || s[7] != '-' || (s[10] != ' ' && s[10] != 'T') ||
        s[13] != ':' || s[16] != ':') return false;
    for (std::size_t i = 0; i < s.size(); ++i)
        if (i != 4 && i != 7 && i != 10 && i != 13 && i != 16 && (s[i] < '0' || s[i] > '9')) return false;
    auto number = [&](std::size_t begin, std::size_t length) {
        int value = 0;
        for (std::size_t i = begin; i < begin + length; ++i) value = value * 10 + s[i] - '0';
        return value;
    };
    const int year = number(0, 4), month = number(5, 2), day = number(8, 2);
    const int hour = number(11, 2), minute = number(14, 2), second = number(17, 2);
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    const int days[] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (year < 1 || month < 1 || month > 12 || day < 1 || day > days[month - 1] ||
        hour > 23 || minute > 59 || second > 59) return false;
    std::tm clock{};
    clock.tm_year = year - 1900; clock.tm_mon = month - 1; clock.tm_mday = day;
    clock.tm_hour = hour; clock.tm_min = minute; clock.tm_sec = second; clock.tm_isdst = -1;
    output = std::mktime(&clock);
    return output != static_cast<std::time_t>(-1) && clock.tm_year == year - 1900 &&
        clock.tm_mon == month - 1 && clock.tm_mday == day && clock.tm_hour == hour &&
        clock.tm_min == minute && clock.tm_sec == second;
}

inline const ExamSession* findActive(const std::vector<ExamSession>& records, int id) {
    for (const auto& session : records)
        if (session.checkinId == id && !session.endTime) return &session;
    return nullptr;
}

// Quyết định ca nào cần thêm/cập nhật hoàn toàn trong bộ nhớ.
inline std::vector<ExamSession> synchronizationChanges(
    const std::vector<ExamAssignment>& assignments,
    const std::vector<ExamSession>& existing, std::time_t now) {
    SessionIndex ids;
    for (std::size_t i = 0; i < existing.size(); ++i) ids.put(existing[i].checkinId, i);
    std::vector<ExamSession> changes;
    for (const auto& assignment : assignments) {
        std::time_t start, end;
        if (assignment.status != "DA_XEP_BAC_SI" ||
            !parseTime(assignment.session.startTime, start) || start > now) continue;
        std::size_t position;
        const bool found = ids.find(assignment.session.checkinId, position);
        if (found && existing[position].endTime) continue;
        if (!found && (!parseTime(assignment.plannedEnd, end) || end <= start)) continue;
        // Ca tới giờ vẫn được nhận khi server khởi động trễ; một bác sĩ chỉ khám một ca.
        if (!found) {
            bool occupied = false;
            for (const auto& session : existing)
                if (!session.endTime && session.doctorId && !session.doctorId->empty() &&
                    session.doctorId == assignment.session.doctorId) occupied = true;
            for (const auto& session : changes)
                if (!session.endTime && session.doctorId && !session.doctorId->empty() &&
                    session.doctorId == assignment.session.doctorId) occupied = true;
            if (occupied) continue;
        }
        ExamSession result = found ? existing[position] : ExamSession{};
        result.checkinId = assignment.session.checkinId;
        result.patientId = assignment.session.patientId;
        result.department = assignment.session.department;
        result.doctorId = assignment.session.doctorId;
        result.doctorName = assignment.session.doctorName;
        result.startTime = assignment.session.startTime;
        // Nguồn chưa có giờ check-in gốc; giữ giờ đã lưu, chỉ dùng giờ bắt đầu khi thiếu.
        if (!result.checkinTime) result.checkinTime = result.startTime;
        changes.push_back(result);
    }
    return changes;
}

inline bool startsBefore(const ExamSession& a, const ExamSession& b) {
    std::time_t first = 0, second = 0;
    const bool validFirst = parseTime(a.startTime, first), validSecond = parseTime(b.startTime, second);
    if (validFirst != validSecond) return !validFirst;
    if (validFirst && first != second) return first < second;
    return a.checkinId < b.checkinId;
}

// Merge Sort ổn định để sắp các ca đang khám; SQLite không quyết định thứ tự hiển thị.
inline void mergeSessions(std::vector<ExamSession>& records, std::vector<ExamSession>& buffer,
                          std::size_t begin, std::size_t end) {
    if (end - begin < 2) return;
    const std::size_t middle = begin + (end - begin) / 2;
    mergeSessions(records, buffer, begin, middle);
    mergeSessions(records, buffer, middle, end);
    std::size_t left = begin, right = middle;
    for (std::size_t target = begin; target < end; ++target) {
        if (left < middle && (right == end || !startsBefore(records[right], records[left])))
            buffer[target] = records[left++];
        else buffer[target] = records[right++];
    }
    for (std::size_t i = begin; i < end; ++i) records[i] = buffer[i];
}

inline std::vector<ExamSession> activeSessions(const std::vector<ExamSession>& records) {
    std::vector<ExamSession> active;
    for (const auto& session : records) if (!session.endTime) active.push_back(session);
    std::vector<ExamSession> buffer(active.size());
    mergeSessions(active, buffer, 0, active.size());
    return active;
}
}
