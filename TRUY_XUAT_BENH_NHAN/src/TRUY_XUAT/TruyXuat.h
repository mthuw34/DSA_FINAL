#pragma once

#include <string>
#include <sqlite3.h>
#include <vector>

struct HoSoTruyXuat {
    int checkinId;
    int patientId;
    std::string patientName;
    std::string birthDate;
    int age;
    std::string gender;
    std::string hometown;
    std::string address;
    std::string phone;
    int departmentOrder;
    std::string department;
    std::string checkinTime;
    int basePriority;
    int currentPriority;
    std::string lastUpdate;
    bool isTroNangLamSang = false;
};

namespace ThuatToanSapXep {
    int layThuTuKhoa(const std::string& department);
    bool xetUuTien(const HoSoTruyXuat& left, const HoSoTruyXuat& right);
    void tron(
        std::vector<HoSoTruyXuat>& records,
        std::vector<HoSoTruyXuat>& buffer,
        int left,
        int middle,
        int right
    );
    void sapXepTron(
        std::vector<HoSoTruyXuat>& records,
        std::vector<HoSoTruyXuat>& buffer,
        int left,
        int right
    );
}

class QuanLyHangDoi {
public:
    bool taiVaXuLyBenhNhan();
};

class DocDuLieu {
public:
    static bool layDanhSachBenhNhan(
        sqlite3* priorityDatabase,
        std::vector<HoSoTruyXuat>& outRecords
    );
};

class GhiDuLieu {
public:
    static bool ghiDanhSachDaSapXep(
        const std::vector<HoSoTruyXuat>& sortedRecords,
        const char* outputPath
    );
};
