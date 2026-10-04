#pragma once
#include "PatientCore.h"
#include <sqlite3.h>
#include <iostream>

namespace HospitalPersistence {
inline std::string text(sqlite3_stmt* statement, int column) {
    if (column < 0) return "";
    const auto* value = sqlite3_column_text(statement, column);
    return value ? reinterpret_cast<const char*>(value) : "";
}

// SQLite chỉ nạp toàn bộ bảng; tầng PatientCore tạo chỉ mục tìm ID trong bộ nhớ.
inline bool loadPatients(sqlite3* db, std::vector<Patient>& records) {
    records.clear();
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT * FROM patients;", -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << sqlite3_errmsg(db) << '\n'; return false;
    }
    auto column = [&](const char* name) {
        for (int i = 0; i < sqlite3_column_count(statement); ++i)
            if (std::string(sqlite3_column_name(statement, i)) == name) return i;
        return -1;
    };
    const int id = column("id"), name = column("name"), age = column("age"),
        birth = column("birth_date"), phone = column("phone"), gender = column("gender"),
        hometown = column("hometown"), address = column("address"), height = column("height"),
        weight = column("weight"), bmi = column("bmi");
    if (id < 0 || name < 0 || birth < 0) { sqlite3_finalize(statement); return false; }
    int result;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        Patient record;
        record.id = sqlite3_column_int(statement, id);
        record.name = text(statement, name); record.birthDate = text(statement, birth);
        record.age = age < 0 ? 0 : sqlite3_column_int(statement, age);
        record.phone = text(statement, phone); record.gender = text(statement, gender);
        record.hometown = text(statement, hometown); record.address = text(statement, address);
        record.height = height < 0 ? 0 : sqlite3_column_double(statement, height);
        record.weight = weight < 0 ? 0 : sqlite3_column_double(statement, weight);
        record.bmi = bmi < 0 ? 0 : sqlite3_column_double(statement, bmi);
        const int nullable[] = {gender, hometown, address, phone};
        for (int i = 0; i < 4; ++i)
            if (nullable[i] < 0 || sqlite3_column_type(statement, nullable[i]) == SQLITE_NULL)
                record.nullFields |= 1u << i;
        records.push_back(record);
    }
    sqlite3_finalize(statement);
    if (result != SQLITE_DONE) { records.clear(); std::cerr << sqlite3_errmsg(db) << '\n'; return false; }
    return true;
}

// Không lọc, nối bảng hoặc sắp xếp bằng SQL khi nạp check-in.
inline bool loadCheckIns(sqlite3* db, std::vector<CheckInRecord>& records) {
    records.clear();
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT checkin_id, patient_id, department, priority, checkin_time FROM checkins;",
                         -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << sqlite3_errmsg(db) << '\n'; return false;
    }
    int result;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        CheckInRecord record;
        record.id = sqlite3_column_int(statement, 0); record.patientId = sqlite3_column_int(statement, 1);
        record.department = text(statement, 2); record.priority = sqlite3_column_int(statement, 3);
        record.time = text(statement, 4); records.push_back(record);
    }
    sqlite3_finalize(statement);
    if (result != SQLITE_DONE) { records.clear(); std::cerr << sqlite3_errmsg(db) << '\n'; return false; }
    return true;
}
}
