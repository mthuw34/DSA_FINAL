#pragma once
#include "Appointment.h"
#include <string>
#include <list>
#include <vector>
#include <unordered_map>

class ClinicQueueManager {
private:
    std::vector<std::list<Appointment>> waitlists;
    std::unordered_map<std::string, std::list<Appointment>::iterator> locator;

public:
    // Khai báo constructor
    ClinicQueueManager();

    // Khai báo hàm gọi bệnh nhân tiếp theo trả về chuỗi JSON
    std::string callNextPatientWeb();
    
    // Khai báo hàm thêm bệnh nhân (chút nữa bạn sẽ viết logic bên file cpp)
    void addPatientWeb(const Appointment& appt);
};