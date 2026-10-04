#include "PatientCore.h"
#include <cassert>

int main() {
    // Buộc tất cả khóa cùng bucket để kiểm tra va chạm, cập nhật và khóa không tồn tại.
    PatientCore::HashIndex index(1);
    for (std::size_t i = 0; i < 1000; ++i) index.put(std::to_string(i), i);
    std::size_t position = 0;
    for (std::size_t i = 0; i < 1000; ++i) {
        assert(index.find(std::to_string(i), position) && position == i);
    }
    index.put("4", 9999);
    assert(index.find("4", position) && position == 9999);
    assert(!index.find("absent", position));
    Patient first, second;
    first.name = "a:b"; first.birthDate = "c";
    second.name = "a"; second.birthDate = "b:c";
    assert(PatientCore::importKey(first) != PatientCore::importKey(second));
    second = first; second.nullFields = 1;
    assert(PatientCore::importKey(first) != PatientCore::importKey(second));

    std::vector<CheckInRecord> records{
        {9, 7, 4, "Khoa Noi", "2026-10-04 09:00:00"},
        {8, 6, 4, "Khoa Noi", "2026-10-04 09:00:00"},
        {5, 6, 4, "Khoa Noi", "2026-10-04 09:00:00"},
        {3, 3, 4, "Khoa Noi", "2026-10-04 08:00:00"},
        {2, 2, 1, "Khoa Noi", "2026-10-04 10:00:00"},
        {1, 1, 5, "Khoa Cap cuu", "2026-10-04 10:00:00"},
    };
    PatientCore::sortCheckIns(records);
    const int expected[] = {1, 2, 3, 5, 8, 9};
    for (std::size_t i = 0; i < records.size(); ++i) assert(records[i].id == expected[i]);
    assert(PatientCore::latestCheckIn(records, 6)->id == 8);
    assert(!PatientCore::latestCheckIn(records, 100));
    assert(PatientCore::findCheckIn(records, 3)->patientId == 3);
    assert(!PatientCore::findCheckIn(records, 100));
    records = {{1, 1, 1, "UnknownB", "same"}, {1, 1, 1, "UnknownA", "same"}};
    PatientCore::sortCheckIns(records);
    assert(records[0].department == "UnknownB"); // Giữ thứ tự khi comparator xem hai khóa bằng nhau.
    records.clear();
    PatientCore::sortCheckIns(records);
}
