#pragma once
#include <string>

using namespace std;

struct HoSoTruyXuat {
    int checkinId;
    int patientId;
    string patientName;
    string birthDate;
    int age;
    string gender;
    string hometown;
    string address;
    string phone;
    int departmentOrder;
    string department;
    string checkinTime;
    int priority;
    int basePriority;
    int currentPriority;
    string lastUpdate;
    bool priorityChanged;
    bool isTroNangLamSang = false;
};