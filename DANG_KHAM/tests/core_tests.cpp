#include "ExamCore.h"
#include <cassert>

int main() {
    std::time_t now;
    assert(ExamCore::parseTime(std::string("2026-10-04 10:00:00"), now));
    std::time_t parsed;
    for (const char* invalid : {"2026-02-29 08:00:00", "2026-13-01 08:00:00",
        "2026-10-04 24:00:00", "2026-10-04 08:60:00", "2026-10-04 08:00:60", "invalid"})
        assert(!ExamCore::parseTime(std::string(invalid), parsed));
    assert(ExamCore::parseTime(std::string("2024-02-29T08:00:00"), parsed));

    auto assignment = [](int id, const char* start, const char* end) {
        ExamAssignment record;
        record.session.checkinId = id; record.session.patientId = id + 100;
        record.session.startTime = start; record.plannedEnd = end;
        record.status = "DA_XEP_BAC_SI";
        return record;
    };
    std::vector<ExamAssignment> assignments{
        assignment(1, "2026-10-04 09:50:00", "2026-10-04 10:30:00"),
        assignment(2, "2026-10-04 09:00:00", "2026-10-04 09:30:00"),
        assignment(3, "2026-10-04 09:50:00", "2026-10-04 10:30:00"),
        assignment(4, "2026-10-04 09:00:00", "2026-10-04 09:30:00"),
        assignment(5, "2026-10-04 10:00:01", "2026-10-04 10:30:00"),
        assignment(6, "2026-10-04 09:00:00", "2026-10-04 10:00:00"),
        assignment(7, "invalid", "2026-10-04 10:30:00"),
        assignment(8, "2026-10-04 09:00:00", "invalid"),
        assignment(9, "2026-10-04 09:00:00", "2026-10-04 10:30:00"),
    };
    assignments.back().status = "CHO_XEP";
    ExamSession overrun = assignments[1].session;
    overrun.diagnosis = "Saved"; overrun.prescription = "Thuoc"; overrun.reminder = "Nhac";
    overrun.checkinTime = "2026-10-04 07:30:00";
    ExamSession completed = assignments[2].session;
    completed.endTime = "2026-10-04 09:59:00";
    std::vector<ExamSession> existing{overrun, completed};
    auto changes = ExamCore::synchronizationChanges(assignments, existing, now);
    assert(changes.size() == 2 && changes[0].checkinId == 1 && changes[1].checkinId == 2);
    assert(changes[1].diagnosis == overrun.diagnosis && changes[1].prescription == overrun.prescription);
    assert(changes[1].reminder == overrun.reminder && changes[1].checkinTime == overrun.checkinTime);
    assert(!changes[1].endTime);
    existing.push_back(changes[0]);
    const auto active = ExamCore::activeSessions(existing);
    assert(active.size() == 2 && active[0].checkinId == 2 && active[1].checkinId == 1);
    assert(ExamCore::findActive(existing, 2) && !ExamCore::findActive(existing, 3));
    assert(ExamCore::activeSessions({}).empty());
    ExamCore::SessionIndex ids;
    ids.put(1, 2); ids.put(4100, 3); // Cùng bucket.
    std::size_t position;
    assert(ids.find(1, position) && position == 2);
    assert(ids.find(4100, position) && position == 3);
    assert(!ids.find(8199, position));
}
