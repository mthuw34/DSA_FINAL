<<<<<<< HEAD:TRUY_XUAT_BENH_NHAN/XuLyDuLieu.cpp
#include "XuLyDuLieu.h"
#include "ThuatToanSapXep.h"
#include <cstdio>

using namespace std;

bool XuLyDuLieu::layDanhSachBenhNhan(sqlite3* sourceDatabase, vector<HoSoTruyXuat>& outRecords) {
    const char* query = R"(
        SELECT c.checkin_id, c.patient_id, p.name, p.birth_date, p.age,
               p.gender, p.hometown, p.address, p.phone,
               CASE c.department
                   WHEN 'Khoa Cap cuu' THEN 1 WHEN 'Khoa Noi' THEN 2
                   WHEN 'Khoa Ngoai' THEN 3 WHEN 'Khoa Tim mach' THEN 4
                   WHEN 'Khoa Nhi' THEN 5 WHEN 'Khoa San' THEN 6
                   WHEN 'Khoa Tai Mui Hong' THEN 7 WHEN 'Khoa Mat' THEN 8
                   WHEN 'Khoa Da lieu' THEN 9 WHEN 'Khoa Than kinh' THEN 10
               END,
               c.department, c.checkin_time, c.priority
        FROM checkins AS c
        JOIN patients AS p ON p.id = c.patient_id
        WHERE c.department IN (
            'Khoa Cap cuu', 'Khoa Noi', 'Khoa Ngoai', 'Khoa Tim mach',
            'Khoa Nhi', 'Khoa San', 'Khoa Tai Mui Hong', 'Khoa Mat',
            'Khoa Da lieu', 'Khoa Than kinh'
        );
    )";

    sqlite3_stmt* sourceStatement = nullptr;
    if (sqlite3_prepare_v2(sourceDatabase, query, -1, &sourceStatement, nullptr) != SQLITE_OK) {
        sqlite3_finalize(sourceStatement);
        return false;
    }

    int sourceResult = SQLITE_ROW;
    while ((sourceResult = sqlite3_step(sourceStatement)) == SQLITE_ROW) {
        HoSoTruyXuat record;
        record.checkinId = sqlite3_column_int(sourceStatement, 0);
        record.patientId = sqlite3_column_int(sourceStatement, 1);
        record.patientName = reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 2));
        record.birthDate = sqlite3_column_type(sourceStatement, 3) == SQLITE_NULL
            ? ""
            : reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 3));
        record.age = sqlite3_column_int(sourceStatement, 4);
        record.gender = sqlite3_column_type(sourceStatement, 5) == SQLITE_NULL
            ? ""
            : reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 5));
        record.hometown = sqlite3_column_type(sourceStatement, 6) == SQLITE_NULL
            ? ""
            : reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 6));
        record.address = sqlite3_column_type(sourceStatement, 7) == SQLITE_NULL
            ? ""
            : reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 7));
        record.phone = sqlite3_column_type(sourceStatement, 8) == SQLITE_NULL
            ? ""
            : reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 8));
        
        record.departmentOrder = sqlite3_column_int(sourceStatement, 9);
        record.department = reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 10));
        
        // Đề phòng SQL lấy thiếu, dùng C++ để gán lại thứ tự
        if(record.departmentOrder == 0) {
             record.departmentOrder = ThuatToanSapXep::layThuTuKhoa(record.department);
        }

        record.checkinTime = reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 11));
        record.priority = sqlite3_column_int(sourceStatement, 12);
        outRecords.push_back(record);
    }

    bool success = (sourceResult == SQLITE_DONE);
    sqlite3_finalize(sourceStatement);
    return success;
}

bool XuLyDuLieu::xuatDuLieuDaSapXep(const vector<HoSoTruyXuat>& sortedRecords, const char* outputPath) {
    sqlite3* outputDatabase = nullptr;

    if (sqlite3_open_v2(
            outputPath,
            &outputDatabase,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
            nullptr
        ) != SQLITE_OK) {
        if (outputDatabase != nullptr) {
            sqlite3_close(outputDatabase);
        }
        return false;
    }

    const char* tableNames[] = {
        "queue_khoa_cap_cuu", "queue_khoa_noi", "queue_khoa_ngoai",
        "queue_khoa_tim_mach", "queue_khoa_nhi", "queue_khoa_san",
        "queue_khoa_tai_mui_hong", "queue_khoa_mat", "queue_khoa_da_lieu",
        "queue_khoa_than_kinh"
    };

    const char* dropTablesSql = R"(
        PRAGMA foreign_keys = ON;
        DROP TABLE IF EXISTS retrieval_queue;
        DROP TABLE IF EXISTS departments;
        DROP TABLE IF EXISTS queue_khoa_cap_cuu;
        DROP TABLE IF EXISTS queue_khoa_noi;
        DROP TABLE IF EXISTS queue_khoa_ngoai;
        DROP TABLE IF EXISTS queue_khoa_tim_mach;
        DROP TABLE IF EXISTS queue_khoa_nhi;
        DROP TABLE IF EXISTS queue_khoa_san;
        DROP TABLE IF EXISTS queue_khoa_tai_mui_hong;
        DROP TABLE IF EXISTS queue_khoa_mat;
        DROP TABLE IF EXISTS queue_khoa_da_lieu;
        DROP TABLE IF EXISTS queue_khoa_than_kinh;
    )";

    char* errorMessage = nullptr;
    if (sqlite3_exec(outputDatabase, dropTablesSql, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        sqlite3_free(errorMessage);
        sqlite3_close(outputDatabase);
        return false;
    }

    const char* createTableFormat = R"(
        CREATE TABLE %s (
            retrieval_order INTEGER PRIMARY KEY,
            checkin_id INTEGER NOT NULL UNIQUE,
            patient_id INTEGER NOT NULL,
            patient_name TEXT NOT NULL,
            birth_date TEXT,
            age INTEGER,
            gender TEXT,
            hometown TEXT,
            address TEXT,
            phone TEXT,
            department_order INTEGER NOT NULL,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            priority INTEGER NOT NULL
        );
    )";

    for (const char* tableName : tableNames) {
        char createTableSql[1024];
        snprintf(createTableSql, sizeof(createTableSql), createTableFormat, tableName);
        if (sqlite3_exec(outputDatabase, createTableSql, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
            sqlite3_free(errorMessage);
            sqlite3_close(outputDatabase);
            return false;
        }
    }

    const char* insertFormat = "INSERT INTO %s VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* departmentStatements[10] = {};
    for (int index = 0; index < 10; ++index) {
        char insertSql[256];
        snprintf(insertSql, sizeof(insertSql), insertFormat, tableNames[index]);
        if (sqlite3_prepare_v2(outputDatabase, insertSql, -1, &departmentStatements[index], nullptr) != SQLITE_OK) {
            for (sqlite3_stmt* statement : departmentStatements) {
                if (statement) sqlite3_finalize(statement);
            }
            sqlite3_close(outputDatabase);
            return false;
        }
    }

    bool success = true;
    int departmentOrders[10] = {};
    for (const HoSoTruyXuat& record : sortedRecords) {
        if (record.departmentOrder < 1 || record.departmentOrder > 10) continue;
        
        sqlite3_stmt* departmentStatement = departmentStatements[record.departmentOrder - 1];
        sqlite3_reset(departmentStatement);
        sqlite3_clear_bindings(departmentStatement);
        sqlite3_bind_int(departmentStatement, 1, ++departmentOrders[record.departmentOrder - 1]);
        sqlite3_bind_int(departmentStatement, 2, record.checkinId);
        sqlite3_bind_int(departmentStatement, 3, record.patientId);
        sqlite3_bind_text(departmentStatement, 4, record.patientName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(departmentStatement, 5, record.birthDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(departmentStatement, 6, record.age);
        sqlite3_bind_text(departmentStatement, 7, record.gender.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(departmentStatement, 8, record.hometown.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(departmentStatement, 9, record.address.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(departmentStatement, 10, record.phone.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(departmentStatement, 11, record.departmentOrder);
        sqlite3_bind_text(departmentStatement, 12, record.department.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(departmentStatement, 13, record.checkinTime.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(departmentStatement, 14, record.priority);
        
        if (sqlite3_step(departmentStatement) != SQLITE_DONE) {
            success = false;
            break;
        }
    }

    for (sqlite3_stmt* statement : departmentStatements) {
        if (statement) sqlite3_finalize(statement);
    }
    sqlite3_close(outputDatabase);
    return success;
}
=======
#include "ClinicQueueManager.h"
#include <fstream>
#include <sqlite3.h>
#include <stdexcept>
#include <nlohmann/json.hpp>    

using json = nlohmann::json;

//Định nghĩa Constructor
ClinicQueueManager::ClinicQueueManager() {
    waitlists.resize(6); // 1 đến 5, bỏ qua index 0
    loadPatientsFromDatabase();
}

void ClinicQueueManager::loadPatientsFromDatabase() {
    const char* databasePath = "QUAN_LY_BENH_NHAN/hospital.db";
    std::ifstream databaseFile(databasePath);
    if (!databaseFile.good()) {
        return;
    }

    sqlite3* database = nullptr;
    if (sqlite3_open_v2(databasePath, &database, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        if (database != nullptr) {
            sqlite3_close(database);
        }
        return;
    }

    const char* query = R"(
        SELECT checkin_id, patient_id, priority, checkin_time
        FROM checkins
        ORDER BY priority ASC, checkin_time ASC, checkin_id ASC;
    )";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, query, -1, &statement, nullptr) == SQLITE_OK) {
        while (sqlite3_step(statement) == SQLITE_ROW) {
            Appointment appointment;
            const char* appt_text = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
            const char* patient_text = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));

            appointment.appointment_code = (appt_text != nullptr) ? std::string(appt_text) : "UNKNOWN";
            appointment.patient_code = (patient_text != nullptr) ? std::string(patient_text) : "UNKNOWN";

            // Mức ưu tiên (cột 2) là số nguyên nên vẫn dùng column_int bình thường
            appointment.priority_level = sqlite3_column_int(statement, 2);
            appointment.checkin_time = 0; 
            appointment.status = "waiting";

            addPatientWeb(appointment);
        }
    }

    sqlite3_finalize(statement);
    sqlite3_close(database);
}

//Định nghĩa hàm gọi bệnh nhân
std::string ClinicQueueManager::callNextPatientWeb() {
    json response;

    try {
        std::lock_guard<std::mutex> lock(queue_mutex);

        for (int i = 1; i <= 5; ++i) {
            if (!waitlists[i].empty()) {
                Appointment next_patient = waitlists[i].front();
                
                // Xóa khỏi hàng chờ và bảng băm
                locator.erase(next_patient.appointment_code);
                waitlists[i].pop_front(); 
                
                // Đóng gói JSON
                response["status"] = "success";
                response["data"] = {
                    {"appointment_code", next_patient.appointment_code},
                    {"patient_code", next_patient.patient_code},
                    {"priority_level", next_patient.priority_level}
                };
                return response.dump();
            }
        }
        
        response["status"] = "empty";
        response["message"] = "Hien tai khong co benh nhan nao dang cho.";
        return response.dump();

    } catch (const std::exception& e) {
        response["status"] = "error";
        response["message"] = e.what();
        return response.dump();
    }
}

//Định nghĩa hàm thêm bệnh nhân
void ClinicQueueManager::addPatientWeb(const Appointment& appt) {
    std::lock_guard<std::mutex> lock(queue_mutex);

    if (appt.priority_level >= 1 && appt.priority_level <= 5) {
        waitlists[appt.priority_level].push_back(appt);
        
        // Lưu iterator vào bảng băm
        auto it = std::prev(waitlists[appt.priority_level].end());
        locator[appt.appointment_code] = it;
    }
}
>>>>>>> fbbd61a3fe61e2c993e3a3a7b581933e1de1aea1:TRUY_XUAT_BENH_NHAN/ClinicQueueManager.cpp
