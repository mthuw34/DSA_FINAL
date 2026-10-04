#include "ThoiGian.h"
#include <iomanip>
#include <sstream>
using namespace std;

namespace {
    tm layLocalTm(time_t t) {
        tm result{};
#ifdef _WIN32
        localtime_s(&result, &t);
#else
        localtime_r(&t, &result);
#endif
        return result;
    }

    time_t taoThoiGianCungNgay(time_t goc, int hour, int minute) {
        tm tm = layLocalTm(goc);
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = 0;
        return mktime(&tm);
    }

    int phutTrongNgay(time_t t) {
        tm tm = layLocalTm(t);
        return tm.tm_hour * 60 + tm.tm_min;
    }

    int thuTrongTuan(time_t t) {
        // 0 = Chủ nhật, 1 = Thứ hai, 2 = Thứ ba, ..., 6 = Thứ bảy
        return layLocalTm(t).tm_wday;
    }
}

namespace ThoiGian {
    time_t HienTai() {
        return time(nullptr);
    }

    string DinhDang(time_t t) {
        tm tm = layLocalTm(t);
        ostringstream out;
        out << put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return out.str();
    }

    string DinhDangNgay(time_t t) {
        tm tm = layLocalTm(t);
        ostringstream out;
        out << put_time(&tm, "%Y-%m-%d");
        return out.str();
    }

    bool LaCuoiTuan(time_t t) {
        int thu = thuTrongTuan(t);
        return thu == 0 || thu == 6;
    }

    bool DangTrongCaThuong(time_t t) {
        if (LaCuoiTuan(t)) return false;

        int p = phutTrongNgay(t);
        return (p >= 7 * 60 && p < 11 * 60 + 30) ||
               (p >= 13 * 60 && p < 17 * 60);
    }

    time_t CaThuongTiepTheo(time_t t) {
        int thu = thuTrongTuan(t);
        int p = phutTrongNgay(t);

        // Nếu đang cuối tuần -> tìm 07:00 thứ 2 kế tiếp
        if (thu == 6) { // Thứ 7
            tm tm = layLocalTm(t);
            tm.tm_mday += 2;
            tm.tm_hour = 7;
            tm.tm_min = 0;
            tm.tm_sec = 0;
            return mktime(&tm);
        }

        if (thu == 0) { // Chủ nhật
            tm tm = layLocalTm(t);
            tm.tm_mday += 1;
            tm.tm_hour = 7;
            tm.tm_min = 0;
            tm.tm_sec = 0;
            return mktime(&tm);
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

        // Sau 17:00 -> ngày làm việc kế tiếp
        tm tm = layLocalTm(t);
        tm.tm_mday += 1;
        tm.tm_hour = 7;
        tm.tm_min = 0;
        tm.tm_sec = 0;

        time_t next = mktime(&tm);
        while (LaCuoiTuan(next)) {
            tm = layLocalTm(next);
            tm.tm_mday += 1;
            tm.tm_hour = 7;
            tm.tm_min = 0;
            tm.tm_sec = 0;
            next = mktime(&tm);
        }
        return next;
    }

    time_t DieuChinhThoiGianKhoaThuong(time_t t) {
        return CaThuongTiepTheo(t);
    }

    bool DuThoiGianKhamKhoaThuong(time_t batDau, int soPhut) {
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

    bool DangTrucCapCuu(time_t t, int loaiCa) {
        tm local = layLocalTm(t);
        const int p = local.tm_hour * 60 + local.tm_min;

        if (loaiCa == 0) {
            return p >= 6 * 60 && p < 18 * 60;
        }

        tm ngayCa = local;
        if (p < 6 * 60) {
            ngayCa.tm_mday -= 1;
            mktime(&ngayCa);
        }

        const int dayIndex = ngayCa.tm_year * 366 + ngayCa.tm_yday;
        const bool dungNgayTruc = (dayIndex % 3) == 0;
        const bool dungKhungGio = (p >= 18 * 60 || p < 6 * 60);
        return dungNgayTruc && dungKhungGio;
    }

    time_t TrucCapCuuTiepTheo(time_t t, int loaiCa) {
        if (DangTrucCapCuu(t, loaiCa)) return t;

        tm local = layLocalTm(t);
        const int p = phutTrongNgay(t);

        if (loaiCa == 0) {
            if (p < 6 * 60) return taoThoiGianCungNgay(t, 6, 0);
            local.tm_mday += 1;
            local.tm_hour = 6;
            local.tm_min = 0;
            local.tm_sec = 0;
            return mktime(&local);
        }

        // Sau 06:00 thi ca dem hien tai da ket thuc, phai bat dau tim
        // tu ngay ke tiep de bao dam it nhat 48 gio nghi.
        if (p < 6 * 60) {
            local.tm_mday -= 1;
        } else {
            local.tm_mday += 1;
        }

        local.tm_hour = 18;
        local.tm_min = 0;
        local.tm_sec = 0;

        for (int i = 0; i < 4; ++i) {
            mktime(&local);
            const int dayIndex = local.tm_year * 366 + local.tm_yday;
            if ((dayIndex % 3) == 0) return mktime(&local);
            local.tm_mday += 1;
            local.tm_hour = 18;
            local.tm_min = 0;
            local.tm_sec = 0;
        }
        return mktime(&local);
    }

}