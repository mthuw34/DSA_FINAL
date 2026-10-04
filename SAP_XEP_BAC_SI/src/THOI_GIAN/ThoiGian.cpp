#include "ThoiGian.h"
#include <iomanip>
#include <sstream>

namespace {
    std::tm layLocalTm(std::time_t t) {
        std::tm result{};
#ifdef _WIN32
        localtime_s(&result, &t);
#else
        localtime_r(&t, &result);
#endif
        return result;
    }

    std::time_t taoThoiGianCungNgay(std::time_t goc, int hour, int minute) {
        std::tm tm = layLocalTm(goc);
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = 0;
        return std::mktime(&tm);
    }

    int phutTrongNgay(std::time_t t) {
        std::tm tm = layLocalTm(t);
        return tm.tm_hour * 60 + tm.tm_min;
    }

    int thuTrongTuan(std::time_t t) {
        // 0 = Chủ nhật, 1 = Thứ 2, ..., 6 = Thứ 7
        return layLocalTm(t).tm_wday;
    }
}

namespace ThoiGian {
    std::time_t HienTai() {
        return std::time(nullptr);
    }

    std::string DinhDang(std::time_t t) {
        std::tm tm = layLocalTm(t);
        std::ostringstream out;
        out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return out.str();
    }

    std::string DinhDangNgay(std::time_t t) {
        std::tm tm = layLocalTm(t);
        std::ostringstream out;
        out << std::put_time(&tm, "%Y-%m-%d");
        return out.str();
    }

    bool LaCuoiTuan(std::time_t t) {
        int thu = thuTrongTuan(t);
        return thu == 0 || thu == 6;
    }

    bool DangTrongCaThuong(std::time_t t) {
        if (LaCuoiTuan(t)) return false;

        int p = phutTrongNgay(t);
        return (p >= 7 * 60 && p < 11 * 60 + 30) ||
               (p >= 13 * 60 && p < 17 * 60);
    }

    std::time_t CaThuongTiepTheo(std::time_t t) {
        int thu = thuTrongTuan(t);
        int p = phutTrongNgay(t);

        // Nếu đang cuối tuần -> tìm 07:00 thứ 2 kế tiếp.
        if (thu == 6) { // Thứ 7
            std::tm tm = layLocalTm(t);
            tm.tm_mday += 2;
            tm.tm_hour = 7;
            tm.tm_min = 0;
            tm.tm_sec = 0;
            return std::mktime(&tm);
        }

        if (thu == 0) { // Chủ nhật
            std::tm tm = layLocalTm(t);
            tm.tm_mday += 1;
            tm.tm_hour = 7;
            tm.tm_min = 0;
            tm.tm_sec = 0;
            return std::mktime(&tm);
        }

        if (p < 7 * 60) {
            return taoThoiGianCungNgay(t, 7, 0);
        }

        if (p >= 7 * 60 && p < 11 * 60 + 30) {
            return t;
        }

        if (p < 13 * 60) {
            return taoThoiGianCungNgay(t, 13, 0);
        }

        if (p >= 13 * 60 && p < 17 * 60) {
            return t;
        }

        // Sau 17:00 -> ngày làm việc kế tiếp.
        std::tm tm = layLocalTm(t);
        tm.tm_mday += 1;
        tm.tm_hour = 7;
        tm.tm_min = 0;
        tm.tm_sec = 0;

        std::time_t next = std::mktime(&tm);
        while (LaCuoiTuan(next)) {
            tm = layLocalTm(next);
            tm.tm_mday += 1;
            tm.tm_hour = 7;
            tm.tm_min = 0;
            tm.tm_sec = 0;
            next = std::mktime(&tm);
        }
        return next;
    }

    std::time_t DieuChinhThoiGianKhoaThuong(std::time_t t) {
        return CaThuongTiepTheo(t);
    }

    bool DuThoiGianKhamKhoaThuong(std::time_t batDau, int soPhut) {
        if (LaCuoiTuan(batDau)) return false;

        int p = phutTrongNgay(batDau);
        int ketThuc = p + soPhut;

        if (p >= 7 * 60 && p < 11 * 60 + 30) {
            return ketThuc <= 11 * 60 + 30;
        }

        if (p >= 13 * 60 && p < 17 * 60) {
            return ketThuc <= 17 * 60;
        }

        return false;
    }

    bool DangTrucCapCuu(std::time_t t, int ngayBatDauTruc) {
        // Quy ước đơn giản: mỗi ngày 00:00 -> 24:00 là một ngày trực/nghỉ.
        // ngayBatDauTruc = 0: hôm nay trực; 1: hôm nay nghỉ.
        std::tm tm = layLocalTm(t);

        // Dùng số ngày từ một mốc cố định để tạo chu kỳ 24h/24h.
        // tm_yday thay đổi theo năm nên cộng thêm năm hiện tại vào pha.
        // Với mục đích mô phỏng, lấy ngày trong năm + pha là đủ.
        int dayIndex = tm.tm_yday;
        return ((dayIndex + ngayBatDauTruc) % 2) == 0;
    }

    std::time_t TrucCapCuuTiepTheo(std::time_t t, int ngayBatDauTruc) {
        if (DangTrucCapCuu(t, ngayBatDauTruc)) return t;

        std::tm tm = layLocalTm(t);
        tm.tm_mday += 1;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        std::time_t next = std::mktime(&tm);

        while (!DangTrucCapCuu(next, ngayBatDauTruc)) {
            tm = layLocalTm(next);
            tm.tm_mday += 1;
            tm.tm_hour = 0;
            tm.tm_min = 0;
            tm.tm_sec = 0;
            next = std::mktime(&tm);
        }

        return next;
    }
}
