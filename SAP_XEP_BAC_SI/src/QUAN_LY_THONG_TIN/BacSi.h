#pragma once
#include <string>
#include <ctime>
using namespace std;

struct BacSi {
    string id;
    string name;
    string BirthDay;
    int age = 0;
    string gender;
    string hometown;
    string address;
    string phone;
    int ExpYears = 0;
    string khoaChuyenMon;

    // Thời điểm bác sĩ có thể nhận bệnh nhân tiếp theo trong mô phỏng
    time_t ThoiGianRanh = 0;

    // Nếu bác sĩ bận đột xuất, bác sĩ sẽ không nhận bệnh nhân cho tới thời điểm này
    time_t TamNghiDen = 0;

    // Mốc kết thúc nghỉ sau ca đêm (tối thiểu 48 giờ)
    time_t NghiSauCaDemDen = 0;

    bool DangLamViec = false;
    bool TrucThuCong = false;
};
