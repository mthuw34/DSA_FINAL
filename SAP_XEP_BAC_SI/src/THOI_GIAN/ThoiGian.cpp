#include "ThoiGian.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <stdexcept>
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

    int phaHopLe(int pha) { return (pha % 6 + 6) % 6; }

    // Chi so ngay dan su, khong reset vao dau nam hay phu thuoc DST.
    long long chiSoNgay(time_t t) {
        auto date = layLocalTm(t);
        long long year = date.tm_year + 1900;
        int month = date.tm_mon + 1;
        year -= month <= 2;
        const auto era = year / 400;
        const auto yoe = year - era * 400;
        const auto doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + date.tm_mday - 1;
        return era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy;
    }

    bool ngayTruc(time_t t, int pha) {
        return (chiSoNgay(t) + phaHopLe(pha) % 2) % 2 == 0;
    }
}

namespace ThoiGian {
    vector<CaTruc> LichTruc(time_t tuNgay, bool capCuu, int pha, int soNgay) {
        vector<CaTruc> result;
        auto date = layLocalTm(tuNgay);
        date.tm_hour = 0; date.tm_min = 0; date.tm_sec = 0;
        for (int i = 0; i < soNgay; ++i) {
            date.tm_isdst = -1;
            const auto midnight = mktime(&date);
            const auto day = DinhDangNgay(midnight);
            if (capCuu && ngayTruc(midnight, pha)) {
                const int hour = (phaHopLe(pha) / 2) * 8;
                result.push_back({day, taoThoiGianCungNgay(midnight, hour, 0),
                    taoThoiGianCungNgay(midnight, hour + 8, 0)});
            } else if (!capCuu && !LaCuoiTuan(midnight)) {
                result.push_back({day, taoThoiGianCungNgay(midnight, 7, 0), taoThoiGianCungNgay(midnight, 11, 30)});
                result.push_back({day, taoThoiGianCungNgay(midnight, 13, 0), taoThoiGianCungNgay(midnight, 17, 0)});
            }
            ++date.tm_mday;
        }
        if (!KiemTraLichTruc(result)) throw invalid_argument("Lich truc khong hop le");
        return result;
    }
    bool KiemTraLichTruc(const vector<CaTruc>& lich) {
        auto sorted = lich;
        sort(sorted.begin(), sorted.end(), [](const CaTruc& a, const CaTruc& b) { return a.batDau < b.batDau; });
        for (size_t i = 0; i < sorted.size(); ++i) {
            const auto& ca = sorted[i];
            if (ca.batDau >= ca.ketThuc || difftime(ca.ketThuc, ca.batDau) > 12 * 3600 ||
                ca.ngay != DinhDangNgay(ca.batDau)) return false;
            if (i && ca.batDau < sorted[i - 1].ketThuc) return false;
        }
        return true;
    }
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

    bool DangTrucCapCuu(time_t t, int ngayBatDauTruc) {
        const int start = phaHopLe(ngayBatDauTruc) / 2 * 8 * 60;
        const int minutes = phutTrongNgay(t);
        return ngayTruc(t, ngayBatDauTruc) && minutes >= start && minutes < start + 8 * 60;
    }

    time_t TrucCapCuuTiepTheo(time_t t, int ngayBatDauTruc) {
        if (DangTrucCapCuu(t, ngayBatDauTruc)) return t;

        const int hour = phaHopLe(ngayBatDauTruc) / 2 * 8;
        auto date = layLocalTm(t);
        for (;;) {
            date.tm_hour = hour; date.tm_min = 0; date.tm_sec = 0; date.tm_isdst = -1;
            const auto next = mktime(&date);
            if (next >= t && ngayTruc(next, ngayBatDauTruc)) return next;
            ++date.tm_mday;
        }
    }

    bool DuThoiGianKhamCapCuu(time_t batDau, int soPhut, int pha) {
        if (soPhut <= 0 || soPhut > 8 * 60 || !DangTrucCapCuu(batDau, pha)) return false;
        const auto end = taoThoiGianCungNgay(batDau, phaHopLe(pha) / 2 * 8 + 8, 0);
        return difftime(end, batDau) >= soPhut * 60;
    }
}
