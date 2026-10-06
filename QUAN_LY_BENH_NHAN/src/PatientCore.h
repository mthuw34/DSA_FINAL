#pragma once
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
#include "../../include/LinearSearch.h"
#include "../../include/HashSearch.h"

struct Patient {
    int id = 0;
    std::string name;
    int age = 0;
    std::string phone, birthDate, gender, hometown, address;
    double height = 0, weight = 0, bmi = 0;
    unsigned nullFields = 0;
};

struct CheckInRecord {
    int id = 0, patientId = 0, priority = 0;
    std::string department, time;
};

namespace PatientCore {

// Bảng băm tự cài đặt, giải quyết va chạm bằng các danh sách trong từng bucket.
// Giá trị là vị trí bản ghi trong vector; không dùng index của SQLite để tìm kiếm.
using HashIndex = DsaSearch::HashStringIndex;

inline HashIndex indexPatients(const std::vector<Patient>& records) {
    HashIndex ids;
    for (std::size_t i = 0; i < records.size(); ++i) ids.put(std::to_string(records[i].id), i);
    return ids;
}

// Tim mot benh nhan trong snapshot moi: khong can tao bang bam cho mot lan tra cuu.
inline const Patient* findPatient(const std::vector<Patient>& records, int id) {
    std::size_t position;
    return DsaSearch::linearFind(records, [id](const auto& p) { return p.id == id; }, position)
        ? &records[position] : nullptr;
}

// Ghép khóa có độ dài từng trường để dữ liệu chứa dấu phân cách không bị trùng khóa.
inline std::string importKey(const Patient& patient) {
    std::string key;
    auto append = [&](const std::string& text, bool isNull = false) {
        key += isNull ? "N;" : "S" + std::to_string(text.size()) + ":" + text;
    };
    append(patient.name); append(patient.birthDate);
    append(patient.gender, patient.nullFields & 1);
    append(patient.hometown, patient.nullFields & 2);
    append(patient.address, patient.nullFields & 4);
    append(patient.phone, patient.nullFields & 8);
    return key;
}

inline int departmentOrder(const std::string& name) {
    const char* names[] = {"Khoa Cap cuu", "Khoa Noi", "Khoa Ngoai", "Khoa Tim mach",
        "Khoa Nhi", "Khoa San", "Khoa Tai Mui Hong", "Khoa Mat", "Khoa Da lieu", "Khoa Than kinh"};
    for (int i = 0; i < 10; ++i) if (name == names[i]) return i;
    return 99;
}

// Merge Sort ổn định tự cài đặt, chạy trên vector trong bộ nhớ.
template<class T, class Compare>
void mergeRange(std::vector<T>& records, std::vector<T>& buffer,
                std::size_t begin, std::size_t end, Compare before) {
    if (end - begin < 2) return;
    const std::size_t middle = begin + (end - begin) / 2;
    mergeRange(records, buffer, begin, middle, before);
    mergeRange(records, buffer, middle, end, before);
    std::size_t left = begin, right = middle;
    for (std::size_t target = begin; target < end; ++target) {
        if (left < middle && (right == end || !before(records[right], records[left])))
            buffer[target] = records[left++];
        else buffer[target] = records[right++];
    }
    for (std::size_t i = begin; i < end; ++i) records[i] = buffer[i];
}

inline void sortCheckIns(std::vector<CheckInRecord>& records) {
    std::vector<CheckInRecord> buffer(records.size());
    mergeRange(records, buffer, 0, records.size(), [](const auto& a, const auto& b) {
        const int first = departmentOrder(a.department), second = departmentOrder(b.department);
        if (first != second) return first < second;
        if (a.priority != b.priority) return a.priority < b.priority;
        if (a.time != b.time) return a.time < b.time;
        if (a.patientId != b.patientId) return a.patientId < b.patientId;
        return a.id < b.id;
    });
}

// Tìm check-in mới nhất của bệnh nhân bằng cách duyệt dữ liệu đã nạp.
inline const CheckInRecord* latestCheckIn(const std::vector<CheckInRecord>& records, int patientId) {
    const CheckInRecord* result = nullptr;
    for (const auto& record : records)
        if (record.patientId == patientId && (!result || record.id > result->id)) result = &record;
    return result;
}

inline const CheckInRecord* findCheckIn(const std::vector<CheckInRecord>& records, int id) {
    // Chi tim mot check-in: duyet O(n), giu thu tu va khong can sap xep O(n log n).
    std::size_t position;
    return DsaSearch::linearFind(records, [id](const auto& r) { return r.id == id; }, position)
        ? &records[position] : nullptr;
}
}
