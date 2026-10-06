// Ngăn tệp tiêu đề bị khai báo lặp lại khi được include từ nhiều nơi.
#pragma once

// Cung cấp std::string và std::string_view dùng trong hồ sơ/cấu hình.
#include <string>
// Cung cấp std::array để khai báo danh sách khoa có kích thước cố định.
#include <array>
// Cung cấp std::size_t dùng khi duyệt danh sách.
#include <cstddef>
// Cung cấp std::string_view cho tên khoa không cần sở hữu chuỗi riêng.
#include <string_view>

// Lưu các thông tin cần thiết của một lượt bệnh nhân trong luồng truy xuất.
struct HoSoTruyXuat {
    // Mã lượt khám, dùng để nhận diện duy nhất lượt đăng ký.
    int checkinId;
    // Mã định danh của bệnh nhân.
    int patientId;
    // Tên bệnh nhân; trường này chưa được lấy từ truy vấn priority_checkins hiện tại.
    std::string patientName;
    // Ngày sinh; trường này chưa được lấy từ truy vấn priority_checkins hiện tại.
    std::string birthDate;
    // Tuổi; trường này chưa được lấy từ truy vấn priority_checkins hiện tại.
    int age;
    // Giới tính; trường này chưa được lấy từ truy vấn priority_checkins hiện tại.
    std::string gender;
    // Quê quán; trường này chưa được lấy từ truy vấn priority_checkins hiện tại.
    std::string hometown;
    // Địa chỉ; trường này chưa được lấy từ truy vấn priority_checkins hiện tại.
    std::string address;
    // Số điện thoại; trường này chưa được lấy từ truy vấn priority_checkins hiện tại.
    std::string phone;
    // Số thứ tự khoa trong danh sách cấu hình, dùng để sắp xếp và chọn bảng đích.
    int departmentOrder;
    // Tên khoa hiện quản lý lượt khám.
    std::string department;
    // Thời điểm bệnh nhân đăng ký khám.
    std::string checkinTime;
    // Mức ưu tiên ban đầu của lượt khám.
    int basePriority;
    // Mức ưu tiên đang áp dụng cho lượt khám.
    int currentPriority;
    // Thời điểm cập nhật ưu tiên gần nhất; chuỗi rỗng biểu thị chưa có thời điểm.
    std::string lastUpdate;
    // Cờ đánh dấu tình trạng trở nặng lâm sàng; mặc định không được đánh dấu.
    bool isTroNangLamSang = false;
};

// Khai báo trước để các khai báo hàm có thể dùng kiểu mảng động.
class MangDongBenhNhan;
// Nạp định nghĩa mảng động chứa các hồ sơ bệnh nhân.
#include "MangDongBenhNhan.h"

// Chứa bảng ánh xạ tên khoa, tên bảng lưu trữ và thứ tự khoa.
namespace CauHinhTruyXuat
{

    // Một mục cấu hình liên kết tên khoa với tên bảng SQLite tương ứng.
    struct Khoa {
        // Tên khoa như được lưu trong dữ liệu lượt khám.
        std::string_view ten;
        // Tên bảng hàng đợi của khoa trong truyXuat.db.
        std::string_view tenBang;
    };

    // Danh sách cố định các khoa được chức năng truy xuất hỗ trợ.
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

    // Tìm thứ tự khoa theo tên; trả về 99 nếu tên không có trong cấu hình.
    inline int thuTuKhoa(std::string_view tenKhoa) {
        // Duyệt từng khoa; thứ tự trong hàng đợi đánh số từ 1.
        for (std::size_t index = 0; index < danhSachKhoa.size(); ++index) {
            // Trả thứ tự tương ứng khi tên khoa trùng khớp.
            if (danhSachKhoa[index].ten == tenKhoa) {
                return static_cast<int>(index) + 1;
            }
        }
        // Giá trị 99 đánh dấu khoa không được nhận diện trong cấu hình.
        return 99;
    }

}

// Khai báo các hàm so sánh và sắp xếp hồ sơ bệnh nhân.
namespace ThuatToanSapXep {
    // Trả thứ tự của khoa để thống nhất với danh sách cấu hình.
    int layThuTuKhoa(const std::string& department);
    // So sánh hai hồ sơ theo thứ tự ưu tiên của hàng đợi.
    bool xetUuTien(const HoSoTruyXuat& left, const HoSoTruyXuat& right);
    // Ghép hai đoạn đã sắp xếp [left, middle] và [middle + 1, right].
    void tron(
        MangDongBenhNhan& records,
        MangDongBenhNhan& buffer,
        int left,
        int middle,
        int right
    );
    // Sắp xếp đoạn đóng [left, right] của mảng bằng Merge Sort.
    void sapXepTron(
        MangDongBenhNhan& records,
        MangDongBenhNhan& buffer,
        int left,
        int right
    );
}

// Điều phối việc đọc, sắp xếp và ghi lại hàng đợi bệnh nhân.
class QuanLyHangDoi {
public:
    // Thực hiện trọn luồng truy xuất; trả true khi đọc, sắp xếp và ghi thành công.
    bool taiVaXuLyBenhNhan();
};

// Cung cấp các thao tác đọc/ghi dữ liệu cho cơ sở dữ liệu truy xuất.
class DBTruyXuat {
public:
    // Đọc các lượt khám từ cơ sở dữ liệu ưu tiên vào mảng đầu ra.
    static bool docDanhSachBenhNhan(MangDongBenhNhan& outRecords);
    // Thay nội dung các bảng hàng đợi bằng danh sách đã sắp xếp.
    static bool ghiDanhSachDaSapXep(
        const MangDongBenhNhan& sortedRecords
    );
};
