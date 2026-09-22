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

bool ClinicQueueManager::ensureRetrievalTable(sqlite3* database) {
    const char* objectTypeSql = R"(
        SELECT type
        FROM sqlite_master
        WHERE name = 'retrieval_queue';
    )";
    sqlite3_stmt* objectTypeStatement = nullptr;
    std::string objectType;

    if (sqlite3_prepare_v2(database, objectTypeSql, -1, &objectTypeStatement, nullptr) == SQLITE_OK &&
        sqlite3_step(objectTypeStatement) == SQLITE_ROW) {
        const char* typeText = reinterpret_cast<const char*>(sqlite3_column_text(objectTypeStatement, 0));
        if (typeText != nullptr) {
            objectType = typeText;
        }
    }
    sqlite3_finalize(objectTypeStatement);

    if (objectType == "view") {
        if (sqlite3_exec(database, "DROP VIEW retrieval_queue;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            return false;
        }
    }

    const char* createTableSql = R"(
        CREATE TABLE IF NOT EXISTS retrieval_queue (
            checkin_id INTEGER PRIMARY KEY,
            patient_id INTEGER NOT NULL,
            patient_name TEXT NOT NULL,
            birth_date TEXT NOT NULL,
            age INTEGER NOT NULL,
            gender TEXT,
            hometown TEXT,
            address TEXT,
            phone TEXT,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            priority INTEGER NOT NULL,
            FOREIGN KEY(patient_id) REFERENCES patients(id)
        );

        DELETE FROM retrieval_queue;

        INSERT INTO retrieval_queue (
            checkin_id, patient_id, patient_name, birth_date, age,
            gender, hometown, address, phone, department,
            checkin_time, priority
        )
        SELECT
            c.checkin_id,
            c.patient_id,
            p.name AS patient_name,
            p.birth_date,
            p.age,
            p.gender,
            p.hometown,
            p.address,
            p.phone,
            c.department,
            c.checkin_time,
            c.priority
        FROM checkins AS c
        JOIN patients AS p ON p.id = c.patient_id;

        CREATE TRIGGER IF NOT EXISTS retrieval_queue_after_checkin_insert
        AFTER INSERT ON checkins
        BEGIN
            INSERT INTO retrieval_queue
            SELECT NEW.checkin_id, NEW.patient_id, p.name, p.birth_date, p.age,
                   p.gender, p.hometown, p.address, p.phone,
                   NEW.department, NEW.checkin_time, NEW.priority
            FROM patients AS p
            WHERE p.id = NEW.patient_id;
        END;

        CREATE TRIGGER IF NOT EXISTS retrieval_queue_after_checkin_update
        AFTER UPDATE ON checkins
        BEGIN
            UPDATE retrieval_queue
            SET patient_id = NEW.patient_id,
                patient_name = (SELECT name FROM patients WHERE id = NEW.patient_id),
                birth_date = (SELECT birth_date FROM patients WHERE id = NEW.patient_id),
                age = (SELECT age FROM patients WHERE id = NEW.patient_id),
                gender = (SELECT gender FROM patients WHERE id = NEW.patient_id),
                hometown = (SELECT hometown FROM patients WHERE id = NEW.patient_id),
                address = (SELECT address FROM patients WHERE id = NEW.patient_id),
                phone = (SELECT phone FROM patients WHERE id = NEW.patient_id),
                department = NEW.department,
                checkin_time = NEW.checkin_time,
                priority = NEW.priority
            WHERE checkin_id = NEW.checkin_id;
        END;

        CREATE TRIGGER IF NOT EXISTS retrieval_queue_after_checkin_delete
        AFTER DELETE ON checkins
        BEGIN
            DELETE FROM retrieval_queue WHERE checkin_id = OLD.checkin_id;
        END;

        CREATE TRIGGER IF NOT EXISTS retrieval_queue_after_patient_update
        AFTER UPDATE ON patients
        BEGIN
            UPDATE retrieval_queue
            SET patient_name = NEW.name,
                birth_date = NEW.birth_date,
                age = NEW.age,
                gender = NEW.gender,
                hometown = NEW.hometown,
                address = NEW.address,
                phone = NEW.phone
            WHERE patient_id = NEW.id;
        END;
    )";

    char* errorMessage = nullptr;
    const int result = sqlite3_exec(
        database,
        createTableSql,
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK) {
        if (errorMessage != nullptr) {
            sqlite3_free(errorMessage);
        }
        return false;
    }

    return true;
}

void ClinicQueueManager::loadPatientsFromDatabase() {
    const char* databasePath = "QUAN_LY_BENH_NHAN/hospital.db";
    std::ifstream databaseFile(databasePath);
    if (!databaseFile.good()) {
        return;
    }

    sqlite3* database = nullptr;
    if (sqlite3_open_v2(databasePath, &database, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        if (database != nullptr) {
            sqlite3_close(database);
        }
        return;
    }

    if (!ensureRetrievalTable(database)) {
        sqlite3_close(database);
        return;
    }

    const char* query = R"(
        SELECT checkin_id, patient_id, priority, checkin_time
        FROM retrieval_queue
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
                response["trang_thai"] = "thanh_cong";
                response["du_lieu"] = {
                    {"ma_check_in", next_patient.appointment_code},
                    {"ma_benh_nhan", next_patient.patient_code},
                    {"muc_uu_tien", next_patient.priority_level}
                };
                return response.dump();
            }
        }
        
        response["trang_thai"] = "rong";
        response["thong_bao"] = "Hien tai khong co benh nhan nao dang cho.";
        return response.dump();

    } catch (const std::exception& e) {
        response["trang_thai"] = "loi";
        response["thong_bao"] = e.what();
        return response.dump();
    }
}

std::string ClinicQueueManager::getQueuesByDepartmentWeb() {
    const std::vector<std::string> departments = {
        "Khoa Cap cuu", "Khoa Noi", "Khoa Ngoai", "Khoa Tim mach",
        "Khoa Nhi", "Khoa San", "Khoa Tai Mui Hong", "Khoa Mat",
        "Khoa Da lieu", "Khoa Than kinh"
    };

    json response;
    for (const std::string& department : departments) {
        response[department] = json::array();
    }

    sqlite3* database = nullptr;
    const char* databasePath = "QUAN_LY_BENH_NHAN/hospital.db";
    if (sqlite3_open_v2(databasePath, &database, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        response["trang_thai"] = "loi";
        response["thong_bao"] = "Khong mo duoc co so du lieu.";
        if (database != nullptr) {
            sqlite3_close(database);
        }
        return response.dump();
    }

    if (!ensureRetrievalTable(database)) {
        response["trang_thai"] = "loi";
        response["thong_bao"] = "Khong tao duoc bang truy xuat trong co so du lieu.";
        sqlite3_close(database);
        return response.dump();
    }

    const char* query = R"(
        SELECT checkin_id, patient_id, department
        FROM retrieval_queue
        ORDER BY priority ASC, checkin_time ASC, checkin_id ASC;
    )";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, query, -1, &statement, nullptr) != SQLITE_OK) {
        response["trang_thai"] = "loi";
        response["thong_bao"] = sqlite3_errmsg(database);
        sqlite3_close(database);
        return response.dump();
    }

    while (sqlite3_step(statement) == SQLITE_ROW) {
        const char* departmentText = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
        if (departmentText == nullptr) {
            continue;
        }

        const std::string department = departmentText;
        if (response.contains(department)) {
            response[department].push_back({
                {"ma_check_in", sqlite3_column_int(statement, 0)},
                {"ma_benh_nhan", sqlite3_column_int(statement, 1)}
            });
        }
    }

    sqlite3_finalize(statement);
    sqlite3_close(database);
    response["trang_thai"] = "thanh_cong";
    return response.dump();
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
