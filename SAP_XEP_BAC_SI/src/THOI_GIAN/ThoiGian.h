#pragma once
#include <ctime>
#include <string>
#include <vector>

namespace ThoiGian {
    struct CaTruc {
        std::string ngay;
        std::time_t batDau = 0;
        std::time_t ketThuc = 0;
    };

    std::time_t HienTai();

    std::string DinhDang(std::time_t t);
    std::string DinhDangNgay(std::time_t t);

    bool LaCuoiTuan(std::time_t t);

    // Khoa thường thì sáng: 07:00 -> 11:30, chiều: 13:00 -> 17:00
    bool DangTrongCaThuong(std::time_t t);

    // Trả về thời điểm bắt đầu ca tiếp theo mà khoa thường có thể làm
    std::time_t CaThuongTiepTheo(std::time_t t);

    // Điều chỉnh thời điểm bắt đầu sao cho nằm trong ca làm việc và ko rơi vào cuối tuần
    std::time_t DieuChinhThoiGianKhoaThuong(std::time_t t);

    // Ktra một ca khám có kết thúc trước khi ca hiện tại kết thúc hay ko
    bool DuThoiGianKhamKhoaThuong(std::time_t batDau, int soPhut);

    // Cấp cứu: pha 0/1 trực 06:00-18:00; pha 2/3 trực 18:00-06:00 hôm sau
    // Pha chẵn bắt đầu T2-T4-T6; pha lẻ bắt đầu T3-T5-T7 (3 ca/tuần)
    bool DangTrucCapCuu(std::time_t t, int phaTruc);

    // Tìm thời điểm gần nhất từ t mà nhóm cấp cứu này được phép trực
    std::time_t TrucCapCuuTiepTheo(std::time_t t, int phaTruc);

    // Ktra lượt khám nằm trọn trong ca Cấp cứu 12 giờ, kể cả qua nửa đêm
    bool DuThoiGianKhamCapCuu(std::time_t batDau, int soPhut, int phaTruc);

    // Sinh lịch trực trong soNgay ngày kể từ ngày chứa mốc
    // Khoa thường bỏ cuối tuần; Cấp cứu có nhóm ca ngày/đêm theo ngày trực
    // Bao gồm ca đêm hôm trước nếu ca đó đang tiếp tục tại mốc
    std::vector<CaTruc> LichTruc(
        std::time_t moc,
        bool capCuu,
        int phaTruc,
        int soNgay = 7
    );

    // Ktra các ca có thgian hợp lệ và ko chồng lấn
    bool KiemTraLichTruc(const std::vector<CaTruc>& lich);
}
