#pragma once
#include <string>
#include <ctime>

struct Appointment {
    std::string appointment_code;
    std::string patient_code;
    int priority_level; 
    std::time_t checkin_time;
    std::string status;
};