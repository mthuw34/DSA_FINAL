#include "ThoiGian.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
using namespace std;

namespace {
    // Chuyển đổi time_t sang cấu trúc tm (thời gian địa phương) an toàn
    tm layLocalTm(time_t t) {
        tm result{};
#ifdef _WIN32
        localtime_s(&result, &t);
#else
        localtime_r(&t, &result);
#endif
        return result;
    }

    // Tạo một thời điểm mới trong cùng ngày với giờ và phút được chỉ định
    time_t taoThoiGianCungNgay(time_t goc, int hour, int minute) {
        tm value = layLocalTm(goc);
        value.tm_hour = hour;
        value.tm_min = minute;
        value.tm_sec = 0;
        return mktime(&value);
    }

    // Lấy thời điểm bắt đầu của ngày (00:00:00) từ một thời điểm cho trước
    time_t dauNgay(time_t t) {
        tm value = layLocalTm(t);
        value.tm_hour = 0;
        value.tm_min = 0;
        value.tm_sec = 0;
        return mktime(&value);
    }

    // Tính tổng số phút đã trôi qua kể từ đầu ngày (00:00)
    int phutTrongNgay(time_t t) {
        tm value = layLocalTm(t);
        return value.tm_hour * 60 + value.tm_min;
    }

    // Lấy thứ trong tuần (0 = Chủ nhật, 1 = Thứ hai, ..., 6 = Thứ bảy)
    int thuTrongTuan(time_t t) {
        return layLocalTm(t).tm_wday;
    }

    // Tính số ngày tuyệt đối theo lịch dương để phục vụ tính toán khoảng cách ngày
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

    // Lấy chỉ số ngày tuyệt đối từ một thời điểm time_t
    long long chiSoNgay(time_t t) {
        const tm value = layLocalTm(t);
        return soNgayTuyetDoi(
            value.tm_year + 1900,
            static_cast<unsigned>(value.tm_mon + 1),
            static_cast<unsigned>(value.tm_mday)
        );
    }

    // Chuẩn hóa nhóm/pha trực về phạm vi hợp lệ (0-3)
    int chuanHoaPha(int pha) {
        int result = pha % 4;
        if (result < 0) result += 4;
        return result;
    }

    // Kiểm tra xem một ngày có đúng lịch trực cấp cứu của nhóm/pha hay không
    bool dungNgayCapCuu(time_t t, int phaTruc) {
        const int thu = thuTrongTuan(t);
        const int pha = chuanHoaPha(phaTruc) % 2;

        // Nhóm 0: Thứ 2, 4, 6. Nhóm 1: Thứ 3, 5, 7.
        if (pha == 0) return thu == 1 || thu == 3 || thu == 5;
        return thu == 2 || thu == 4 || thu == 6;
    }

    // Kiểm tra xem pha trực có phải là ca đêm hay không (pha 2 và 3)
    bool caDemCapCuu(int phaTruc) {
        return chuanHoaPha(phaTruc) >= 2;
    }

    // Khởi tạo đối tượng CaTruc cấp cứu cho một ngày và pha trực cụ thể
    ThoiGian::CaTruc caCapCuu(time_t ngay, int phaTruc) {
        const bool caDem = caDemCapCuu(phaTruc);
        const time_t start = taoThoiGianCungNgay(ngay, caDem ? 18 : 6, 0);
        tm end = layLocalTm(start);
        end.tm_hour = caDem ? 6 : 18;
        if (caDem) ++end.tm_mday;
        end.tm_isdst = -1;
        return {ThoiGian::DinhDangNgay(start), start, mktime(&end)};
    }

    // Xác định ca trực cấp cứu tại một thời điểm cụ thể (xử lý ca đêm vắt ngang 2 ngày)
    // Trước 06:00, ca đêm thuộc ngày bắt đầu hôm trước.
    ThoiGian::CaTruc caCapCuuTai(time_t t, int phaTruc) {
        tm day = layLocalTm(t);
        if (caDemCapCuu(phaTruc) && phutTrongNgay(t) < 6 * 60) --day.tm_mday;
        day.tm_isdst = -1;
        return caCapCuu(mktime(&day), phaTruc);
    }
}

namespace ThoiGian {
    // Lấy thời gian hiện tại của hệ thống
    time_t HienTai() {
        return time(nullptr);
    }

    // Định dạng thời gian thành chuỗi (YYYY-MM-DD HH:MM:SS)
    string DinhDang(time_t t) {
        tm value = layLocalTm(t);
        ostringstream out;
        out << put_time(&value, "%Y-%m-%d %H:%M:%S");
        return out.str();
    }

    // Định dạng ngày thành chuỗi (YYYY-MM-DD)
    string DinhDangNgay(time_t t) {
        tm value = layLocalTm(t);
        ostringstream out;
        out << put_time(&value, "%Y-%m-%d");
        return out.str();
    }

    // Kiểm tra xem thời điểm cho trước có phải là cuối tuần (Thứ 7, Chủ nhật) không
    bool LaCuoiTuan(time_t t) {
        const int thu = thuTrongTuan(t);
        return thu == 0 || thu == 6;
    }

    // Kiểm tra xem thời điểm hiện tại có nằm trong ca làm việc hành chính (khoa thường) không
    bool DangTrongCaThuong(time_t t) {
        if (LaCuoiTuan(t)) return false;

        const int p = phutTrongNgay(t);
        return (p >= 7 * 60 && p < 11 * 60 + 30) ||
               (p >= 13 * 60 && p < 17 * 60);
    }

    // Tìm thời điểm bắt đầu ca làm việc hành chính tiếp theo
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

    // Điều chỉnh thời gian về đầu ca làm việc bình thường (nếu ngoài giờ)
    time_t DieuChinhThoiGianKhoaThuong(time_t t) {
        return CaThuongTiepTheo(t);
    }

    // Kiểm tra xem ca làm việc hành chính hiện tại còn đủ thời lượng để khám bệnh hay không
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

    // Kiểm tra xem thời điểm cho trước có nằm trong ca trực cấp cứu của nhóm không
    bool DangTrucCapCuu(time_t t, int phaTruc) {
        const auto shift = caCapCuuTai(t, phaTruc);
        return dungNgayCapCuu(shift.batDau, phaTruc) &&
               t >= shift.batDau && t < shift.ketThuc;
    }

    // Tìm thời điểm bắt đầu ca trực cấp cứu tiếp theo của nhóm
    time_t TrucCapCuuTiepTheo(time_t t, int phaTruc) {
        if (DangTrucCapCuu(t, phaTruc)) return t;

        for (int offset = 0; offset <= 7; ++offset) {
            tm value = layLocalTm(t);
            value.tm_mday += offset;
            value.tm_hour = caDemCapCuu(phaTruc) ? 18 : 6;
            value.tm_min = 0;
            value.tm_sec = 0;
            const time_t start = mktime(&value);

            if (!dungNgayCapCuu(start, phaTruc)) continue;
            if (start >= t) return start;
        }

        return t;
    }

    // Kiểm tra xem ca trực cấp cứu hiện tại còn đủ thời lượng khám bệnh hay không
    bool DuThoiGianKhamCapCuu(time_t batDau, int soPhut, int phaTruc) {
        if (soPhut <= 0 || soPhut > 12 * 60) return false;
        if (!DangTrucCapCuu(batDau, phaTruc)) return false;
        return batDau + static_cast<time_t>(soPhut) * 60 <= caCapCuuTai(batDau, phaTruc).ketThuc;
    }

    // Lập danh sách các ca trực trong một số ngày nhất định kể từ một mốc thời gian
    vector<CaTruc> LichTruc(
        time_t moc,
        bool capCuu,
        int phaTruc,
        int soNgay
    ) {
        vector<CaTruc> result;
        if (soNgay <= 0) return result;

        const time_t firstDay = dauNgay(moc);

        // Bao gồm ca đêm đang tiếp nối từ hôm trước để trạng thái và tăng ca dùng đúng ca
        if (capCuu && DangTrucCapCuu(moc, phaTruc)) {
            const auto current = caCapCuuTai(moc, phaTruc);
            if (current.batDau < firstDay) result.push_back(current);
        }

        for (int offset = 0; offset < soNgay; ++offset) {
            tm day = layLocalTm(firstDay);
            day.tm_mday += offset;
            day.tm_hour = 0;
            day.tm_min = 0;
            day.tm_sec = 0;
            const time_t currentDay = mktime(&day);

            if (capCuu) {
                if (!dungNgayCapCuu(currentDay, phaTruc)) continue;
                result.push_back(caCapCuu(currentDay, phaTruc));
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

    // Kiểm tra tính hợp lệ của lịch trực (không có ca nào bị chồng chéo hoặc sai định dạng thời gian)
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