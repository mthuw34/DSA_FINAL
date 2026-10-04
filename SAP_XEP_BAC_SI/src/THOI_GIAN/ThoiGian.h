#pragma once
#include <ctime>
#include <string>

namespace ThoiGian {
    std::time_t HienTai();

    std::string DinhDang(std::time_t t);
    std::string DinhDangNgay(std::time_t t);

    bool LaCuoiTuan(std::time_t t);

    // Khoa thường:
    // - sáng: 07:00 -> 11:30
    // - chiều: 13:00 -> 17:00
    bool DangTrongCaThuong(std::time_t t);

    // Trả về thời điểm bắt đầu ca tiếp theo mà khoa thường có thể làm.
    std::time_t CaThuongTiepTheo(std::time_t t);

    // Điều chỉnh thời điểm bắt đầu sao cho nằm trong ca làm việc
    // và không rơi vào cuối tuần.
    std::time_t DieuChinhThoiGianKhoaThuong(std::time_t t);

    // Kiểm tra một ca khám có kết thúc trước khi ca hiện tại kết thúc hay không.
    bool DuThoiGianKhamKhoaThuong(std::time_t batDau, int soPhut);

    // Khoa cấp cứu: chia bác sĩ thành 2 nhóm ca.
    // loaiCa = 0: ca ngày 06:00 -> 18:00.
    // loaiCa = 1: ca đêm 18:00 -> 06:00 hôm sau.
    // Nhóm ca đêm trực theo chu kỳ 1 trực - 2 nghỉ.
    bool DangTrucCapCuu(std::time_t t, int loaiCa);

    // Tìm thời điểm bắt đầu ca cấp cứu tiếp theo mà bác sĩ được phép trực.
    std::time_t TrucCapCuuTiepTheo(std::time_t t, int loaiCa);
}
