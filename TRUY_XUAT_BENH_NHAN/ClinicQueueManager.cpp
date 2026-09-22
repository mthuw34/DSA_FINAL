#include "ClinicQueueManager.h"
#include <stdexcept>
#include <nlohmann/json.hpp>    

using json = nlohmann::json;

//Định nghĩa Constructor
ClinicQueueManager::ClinicQueueManager() {
    waitlists.resize(6); // 1 đến 5, bỏ qua index 0
}

//Định nghĩa hàm gọi bệnh nhân
std::string ClinicQueueManager::callNextPatientWeb() {
    json response;

    try {
        for (int i = 5; i >= 1; --i) {
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
    if (appt.priority_level >= 1 && appt.priority_level <= 5) {
        waitlists[appt.priority_level].push_back(appt);
        
        // Lưu iterator vào bảng băm
        auto it = std::prev(waitlists[appt.priority_level].end());
        locator[appt.appointment_code] = it;
    }
}
