#pragma once
#include <ctime>
#include <string>
using namespace std;

namespace ThoiGian {
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

    // Khoa cấp cứu: một bác sĩ làm 24h rồi nghỉ 24h
    // ngayBatDauTruc: 0 là ngày trực, 1 là ngày nghỉ
    bool DangTrucCapCuu(time_t t, int ngayBatDauTruc);

    // Tìm thời điểm trực cấp cứu tiếp theo của bác sĩ
    time_t TrucCapCuuTiepTheo(time_t t, int ngayBatDauTruc);
}