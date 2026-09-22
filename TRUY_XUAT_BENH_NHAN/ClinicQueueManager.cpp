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
    const char* databasePaths[] = {
        "../QUAN_LY_BENH_NHAN/hospital.db",
        "QUAN_LY_BENH_NHAN/hospital.db"
    };

    const char* databasePath = nullptr;
    for (const char* path : databasePaths) {
        std::ifstream databaseFile(path);
        if (databaseFile.good()) {
            databasePath = path;
            break;
        }
    }

    if (databasePath == nullptr) {
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
            appointment.appointment_code = std::to_string(sqlite3_column_int(statement, 0));
            appointment.patient_code = std::to_string(sqlite3_column_int(statement, 1));
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
