#pragma once
#include <ctime>
#include <string>
#include <vector>
using namespace std;

namespace ThoiGian {
    struct CaTruc { string ngay; time_t batDau; time_t ketThuc; };
    vector<CaTruc> LichTruc(time_t tuNgay, bool capCuu, int pha, int soNgay = 7);
    // Khoang ca [batDau, ketThuc): toi da 12 gio, khong trung ca cung bac si.
    bool KiemTraLichTruc(const vector<CaTruc>& lich);
    bool DuThoiGianKhamCapCuu(time_t batDau, int soPhut, int pha);
    time_t HienTai();

    string DinhDang(time_t t);
    string DinhDangNgay(time_t t);

    bool LaCuoiTuan(time_t t);

    // Khoa thường:
    // Sáng: 07:00 -> 11:30
    // Chiều: 13:00 -> 17:00
    bool DangTrongCaThuong(time_t t);

    // Trả về thời điểm bắt đầu ca tiếp theo mà khoa thường có thể làm
    time_t CaThuongTiepTheo(time_t t);

    // Điều chỉnh thời điểm bắt đầu để nằm trong ca làm việc và ko rơi vào cuối tuần
    time_t DieuChinhThoiGianKhoaThuong(time_t t);

    // Kiểm tra 1 ca khám có kết thúc trước khi ca hiện tại kết thúc hay ko
    bool DuThoiGianKhamKhoaThuong(time_t batDau, int soPhut);

    // Khoa cap cuu: 3 ca 8 gio, moi bac si lam mot ca va nghi cach ngay.
    // Pha 0..5: pha / 2 la ca (00, 08, 16 gio), pha % 2 la nhom ngay nghi.
    bool DangTrucCapCuu(time_t t, int ngayBatDauTruc);

    // Tìm thời điểm trực cấp cứu tiếp theo của bác sĩ
    time_t TrucCapCuuTiepTheo(time_t t, int ngayBatDauTruc);
}
