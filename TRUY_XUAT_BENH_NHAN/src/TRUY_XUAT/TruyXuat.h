#pragma once

#include <string>
#include <array>
#include <cstddef>
#include <string_view>
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

namespace CauHinhTruyXuat
{

    struct Khoa {
        std::string_view ten;
        std::string_view tenBang;
    };

    inline constexpr std::array<Khoa, 10> danhSachKhoa = {{
        {"Khoa Cap cuu", "queue_khoa_cap_cuu"},
        {"Khoa Noi", "queue_khoa_noi"},
        {"Khoa Ngoai", "queue_khoa_ngoai"},
        {"Khoa Tim mach", "queue_khoa_tim_mach"},
        {"Khoa Nhi", "queue_khoa_nhi"},
        {"Khoa San", "queue_khoa_san"},
        {"Khoa Tai Mui Hong", "queue_khoa_tai_mui_hong"},
        {"Khoa Mat", "queue_khoa_mat"},
        {"Khoa Da lieu", "queue_khoa_da_lieu"},
        {"Khoa Than kinh", "queue_khoa_than_kinh"}
    }};

    inline int thuTuKhoa(std::string_view tenKhoa) {
        for (std::size_t index = 0; index < danhSachKhoa.size(); ++index) {
            if (danhSachKhoa[index].ten == tenKhoa) {
                return static_cast<int>(index) + 1;
            }
        }
        return 99;
    }

}

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

class DBTruyXuat {
public:
    static bool docDanhSachBenhNhan(std::vector<HoSoTruyXuat>& outRecords);
    static bool ghiDanhSachDaSapXep(
        const std::vector<HoSoTruyXuat>& sortedRecords
    );
};
