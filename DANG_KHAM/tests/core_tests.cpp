#include "ExamCore.h"
#include "../../include/BinarySearch.h"
#include <cassert>

int main() {
    std::vector<int> keys{40, 2, 17, 1, 90};
    DsaSearch::BinaryIdIndex binary(keys, [](int id) { return id; });
    std::size_t original = 123;
    assert(binary.find(1, original) && original == 3);
    assert(binary.find(90, original) && original == 4);
    assert(binary.find(17, original) && original == 2);
    assert(!binary.find(0, original) && !binary.find(18, original) && !binary.find(91, original));
    assert(keys == (std::vector<int>{40, 2, 17, 1, 90}));
    DsaSearch::BinaryIdIndex empty(std::vector<int>{}, [](int id) { return id; });
    assert(!empty.find(1, original));
    DsaSearch::BinaryIdIndex single(std::vector<int>{7}, [](int id) { return id; });
    assert(single.find(7, original) && original == 0 && !single.find(8, original));
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
        record.session.doctorId = "BS" + std::to_string(id);
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
    assert(changes.size() == 4 && changes[0].checkinId == 1 && changes[1].checkinId == 2);
    assert(changes[2].checkinId == 4 && changes[3].checkinId == 6); // Nhận cả ca bị trễ.
    assert(changes[1].diagnosis == overrun.diagnosis && changes[1].prescription == overrun.prescription);
    assert(changes[1].reminder == overrun.reminder && changes[1].checkinTime == overrun.checkinTime);
    assert(!changes[1].endTime);
    auto sameDoctor = assignments[0];
    sameDoctor.session.checkinId = 10;
    sameDoctor.session.doctorId = overrun.doctorId;
    assert(ExamCore::synchronizationChanges({sameDoctor}, existing, now).empty());
    sameDoctor.session.doctorId = assignments[0].session.doctorId;
    assert(ExamCore::synchronizationChanges({assignments[0], sameDoctor}, {}, now).size() == 1);
    auto changedDoctor = assignments[1];
    changedDoctor.session.doctorId = "BS_CHANGED";
    sameDoctor.session.doctorId = changedDoctor.session.doctorId;
    assert(ExamCore::synchronizationChanges({changedDoctor, sameDoctor}, {overrun}, now).size() == 1);
    sameDoctor.session.doctorId = completed.doctorId;
    assert(ExamCore::synchronizationChanges({sameDoctor}, {completed}, now).size() == 1);
    existing.push_back(changes[0]);
    const auto active = ExamCore::activeSessions(existing);
    assert(active.size() == 2 && active[0].checkinId == 2 && active[1].checkinId == 1);
    assert(ExamCore::findActive(existing, 2) && !ExamCore::findActive(existing, 3));
    assert(!ExamCore::findActive(existing, 999) && !ExamCore::findActive({}, 1));
    auto future = changes[0];
    future.startTime = "2999-01-01 08:00:00";
    assert(!ExamCore::findActive({future}, future.checkinId));
    assert(ExamCore::activeSessions({}).empty());
    auto duplicate = overrun;
    duplicate.checkinId = 20;
    duplicate.startTime = "2026-10-04 08:00:00";
    duplicate.diagnosis.reset(); duplicate.prescription.reset(); duplicate.reminder.reset();
    assert(ExamCore::duplicateDoctorAssignments({duplicate, completed, overrun}) == std::vector<int>{20});
    auto noResults = overrun;
    noResults.diagnosis.reset(); noResults.prescription.reset(); noResults.reminder.reset();
    assert(ExamCore::duplicateDoctorAssignments({noResults, duplicate, completed}) == std::vector<int>{2});
    assert(ExamCore::duplicateDoctorAssignments({completed, overrun}).empty());
    ExamCore::SessionIndex ids;
    ids.put(1, 2); ids.put(4100, 3); // Cùng bucket.
    std::size_t position;
    assert(ids.find(1, position) && position == 2);
    assert(ids.find(4100, position) && position == 3);
    assert(!ids.find(8199, position));
    ids.put(1, 99);
    assert(ids.find(1, position) && position == 99);
    DsaSearch::HashIdIndex collisions(1);
    for (int id : {-7, 0, 4, 20000}) collisions.put(id, static_cast<std::size_t>(id + 7));
    for (int id : {-7, 0, 4, 20000})
        assert(collisions.find(id, position) && position == static_cast<std::size_t>(id + 7));
    assert(!collisions.find(8, position));
    DsaSearch::HashIdIndex batch(keys, [](int id) { return id; });
    assert(batch.find(17, position) && position == 2 && !batch.find(18, position));
}
