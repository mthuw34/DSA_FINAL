#include "ThoiGian.h"
#include <algorithm>
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
        tm value = layLocalTm(goc);
        value.tm_hour = hour;
        value.tm_min = minute;
        value.tm_sec = 0;
        return mktime(&value);
    }

    time_t dauNgay(time_t t) {
        tm value = layLocalTm(t);
        value.tm_hour = 0;
        value.tm_min = 0;
        value.tm_sec = 0;
        return mktime(&value);
    }

    int phutTrongNgay(time_t t) {
        tm value = layLocalTm(t);
        return value.tm_hour * 60 + value.tm_min;
    }

    int thuTrongTuan(time_t t) {
        // 0 = Chủ nhật, 1 = Thứ hai, ..., 6 = Thứ bảy.
        return layLocalTm(t).tm_wday;
    }

    // Số ngày tuyệt đối tăng đúng 1 sau mỗi ngày dương lịch.
    long long soNgayTuyetDoi(int year, unsigned month, unsigned day) {
        year -= month <= 2;
        const int era = (year >= 0 ? year : year - 399) / 400;
        const unsigned yoe = static_cast<unsigned>(year - era * 400);
        const unsigned doy =
            (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
        const unsigned doe =
            yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return static_cast<long long>(era) * 146097LL +
               static_cast<long long>(doe);
    }

    long long chiSoNgay(time_t t) {
        const tm value = layLocalTm(t);
        return soNgayTuyetDoi(
            value.tm_year + 1900,
            static_cast<unsigned>(value.tm_mon + 1),
            static_cast<unsigned>(value.tm_mday)
        );
    }

    int chuanHoaPha(int pha) {
        int result = pha % 2;
        if (result < 0) result += 2;
        return result;
    }

    bool dungNgayCapCuu(time_t t, int phaTruc) {
        const int thu = thuTrongTuan(t);
        const int pha = chuanHoaPha(phaTruc);

        // Nhóm 0: Thứ 2, 4, 6. Nhóm 1: Thứ 3, 5, 7.
        if (pha == 0) return thu == 1 || thu == 3 || thu == 5;
        return thu == 2 || thu == 4 || thu == 6;
    }
}

namespace ThoiGian {
    time_t HienTai() {
        return time(nullptr);
    }

    string DinhDang(time_t t) {
        tm value = layLocalTm(t);
        ostringstream out;
        out << put_time(&value, "%Y-%m-%d %H:%M:%S");
        return out.str();
    }

    string DinhDangNgay(time_t t) {
        tm value = layLocalTm(t);
        ostringstream out;
        out << put_time(&value, "%Y-%m-%d");
        return out.str();
    }

    bool LaCuoiTuan(time_t t) {
        const int thu = thuTrongTuan(t);
        return thu == 0 || thu == 6;
    }

    bool DangTrongCaThuong(time_t t) {
        if (LaCuoiTuan(t)) return false;

        const int p = phutTrongNgay(t);
        return (p >= 7 * 60 && p < 11 * 60 + 30) ||
               (p >= 13 * 60 && p < 17 * 60);
    }

    time_t CaThuongTiepTheo(time_t t) {
        const int thu = thuTrongTuan(t);
        const int p = phutTrongNgay(t);

        if (thu == 6) {
            tm value = layLocalTm(t);
            value.tm_mday += 2;
            value.tm_hour = 7;
            value.tm_min = 0;
            value.tm_sec = 0;
            return mktime(&value);
        }

        if (thu == 0) {
            tm value = layLocalTm(t);
            value.tm_mday += 1;
            value.tm_hour = 7;
            value.tm_min = 0;
            value.tm_sec = 0;
            return mktime(&value);
        }

        if (p < 7 * 60) return taoThoiGianCungNgay(t, 7, 0);
        if (p < 11 * 60 + 30) return t;
        if (p < 13 * 60) return taoThoiGianCungNgay(t, 13, 0);
        if (p < 17 * 60) return t;

        tm value = layLocalTm(t);
        value.tm_mday += 1;
        value.tm_hour = 7;
        value.tm_min = 0;
        value.tm_sec = 0;

        time_t next = mktime(&value);
        while (LaCuoiTuan(next)) {
            value = layLocalTm(next);
            value.tm_mday += 1;
            value.tm_hour = 7;
            value.tm_min = 0;
            value.tm_sec = 0;
            next = mktime(&value);
        }
        return next;
    }

    time_t DieuChinhThoiGianKhoaThuong(time_t t) {
        return CaThuongTiepTheo(t);
    }

    bool DuThoiGianKhamKhoaThuong(time_t batDau, int soPhut) {
        if (soPhut <= 0 || LaCuoiTuan(batDau)) return false;

        const int p = phutTrongNgay(batDau);
        const int ketThuc = p + soPhut;

        if (p >= 7 * 60 && p < 11 * 60 + 30)
            return ketThuc <= 11 * 60 + 30;

        if (p >= 13 * 60 && p < 17 * 60)
            return ketThuc <= 17 * 60;

        return false;
    }

    bool DangTrucCapCuu(time_t t, int phaTruc) {
        if (!dungNgayCapCuu(t, phaTruc)) return false;
        const int p = phutTrongNgay(t);
        return p >= 7 * 60 && p < 17 * 60;
    }

    time_t TrucCapCuuTiepTheo(time_t t, int phaTruc) {
        if (DangTrucCapCuu(t, phaTruc)) return t;

        for (int offset = 0; offset <= 7; ++offset) {
            tm value = layLocalTm(t);
            value.tm_mday += offset;
            value.tm_hour = 7;
            value.tm_min = 0;
            value.tm_sec = 0;
            const time_t start = mktime(&value);

            if (!dungNgayCapCuu(start, phaTruc)) continue;
            if (start >= t) return start;
        }

        return t;
    }

    bool DuThoiGianKhamCapCuu(time_t batDau, int soPhut, int phaTruc) {
        if (soPhut <= 0 || soPhut > 10 * 60) return false;
        if (!DangTrucCapCuu(batDau, phaTruc)) return false;
        const time_t endShift = taoThoiGianCungNgay(batDau, 17, 0);
        return batDau + static_cast<time_t>(soPhut) * 60 <= endShift;
    }

    vector<CaTruc> LichTruc(
        time_t moc,
        bool capCuu,
        int phaTruc,
        int soNgay
    ) {
        vector<CaTruc> result;
        if (soNgay <= 0) return result;

        const time_t firstDay = dauNgay(moc);

        for (int offset = 0; offset < soNgay; ++offset) {
            tm day = layLocalTm(firstDay);
            day.tm_mday += offset;
            day.tm_hour = 0;
            day.tm_min = 0;
            day.tm_sec = 0;
            const time_t currentDay = mktime(&day);

            if (capCuu) {
                if (!dungNgayCapCuu(currentDay, phaTruc)) continue;
                const time_t start = taoThoiGianCungNgay(currentDay, 7, 0);
                result.push_back({
                    DinhDangNgay(start),
                    start,
                    taoThoiGianCungNgay(currentDay, 17, 0)
                });
                continue;
            }

            if (LaCuoiTuan(currentDay)) continue;

            const time_t morning = taoThoiGianCungNgay(currentDay, 7, 0);
            const time_t afternoon = taoThoiGianCungNgay(currentDay, 13, 0);

            result.push_back({
                DinhDangNgay(morning),
                morning,
                taoThoiGianCungNgay(currentDay, 11, 30)
            });
            result.push_back({
                DinhDangNgay(afternoon),
                afternoon,
                taoThoiGianCungNgay(currentDay, 17, 0)
            });
        }

        return result;
    }

    bool KiemTraLichTruc(const vector<CaTruc>& lich) {
        for (const auto& shift : lich) {
            if (shift.batDau >= shift.ketThuc) return false;
            if (difftime(shift.ketThuc, shift.batDau) > 12 * 60 * 60)
                return false;
            if (shift.ngay != DinhDangNgay(shift.batDau)) return false;
        }

        for (size_t i = 0; i < lich.size(); ++i) {
            for (size_t j = i + 1; j < lich.size(); ++j) {
                const bool overlap =
                    lich[i].batDau < lich[j].ketThuc &&
                    lich[j].batDau < lich[i].ketThuc;
                if (overlap) return false;
            }
        }

        return true;
    }
}
