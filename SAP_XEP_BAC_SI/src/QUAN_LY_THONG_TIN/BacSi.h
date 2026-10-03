#pragma once
#include <string>
#include <ctime>

struct BacSi {
    std::string id;
    std::string name;
    std::string BirthDay;
    int age = 0;
    std::string gender;
    std::string hometown;
    std::string address;
    std::string phone;
    int ExpYears = 0;
    std::string khoaChuyenMon;

    // Thời điểm bác sĩ có thể nhận bệnh nhân tiếp theo trong mô phỏng.
    std::time_t ThoiGianRanh = 0;

    // Nếu bác sĩ bận đột xuất, bác sĩ sẽ không nhận bệnh nhân
    // cho tới thời điểm này.
    std::time_t TamNghiDen = 0;

    bool DangLamViec = false;
};
