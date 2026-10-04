#include "ThoiGian.h"
#include "ExamCore.h"
#include "QuanLyBacSi.h"
#include <cassert>

int main() {
    time_t monday;
    assert(ExamCore::parseTime(std::string("2026-10-05 09:00:00"), monday));
    auto normal = ThoiGian::LichTruc(monday, false, 0);
    assert(normal.size() == 10); // 5 ngày làm việc, 2 ca mỗi ngày.
    assert(ThoiGian::DinhDang(normal[0].batDau) == "2026-10-05 07:00:00");
    assert(ThoiGian::DinhDang(normal[0].ketThuc) == "2026-10-05 11:30:00");
    assert(ThoiGian::DinhDang(normal[1].batDau) == "2026-10-05 13:00:00");
    assert(ThoiGian::DinhDang(normal[1].ketThuc) == "2026-10-05 17:00:00");
    assert(normal.back().ngay == "2026-10-09");
    assert(ThoiGian::KiemTraLichTruc(normal));
    for (int phase : {0, 1, 2, 3, 4, 5}) {
        const auto emergency = ThoiGian::LichTruc(monday, true, phase);
        assert(emergency.size() == 3 || emergency.size() == 4);
        for (const auto& shift : emergency) {
            assert(ThoiGian::DangTrucCapCuu(shift.batDau, phase));
            assert(ThoiGian::DinhDang(shift.batDau).substr(11) ==
                (phase / 2 == 0 ? "00:00:00" : phase / 2 == 1 ? "08:00:00" : "16:00:00"));
            assert(shift.ngay == ThoiGian::DinhDangNgay(shift.batDau));
            assert(shift.ketThuc > shift.batDau);
            assert(difftime(shift.ketThuc, shift.batDau) == 8 * 3600);
            assert(ThoiGian::DuThoiGianKhamCapCuu(shift.ketThuc - 20 * 60, 20, phase));
            assert(!ThoiGian::DuThoiGianKhamCapCuu(shift.ketThuc - 19 * 60, 20, phase));
            assert(!ThoiGian::DuThoiGianKhamCapCuu(shift.batDau, 481, phase));
            assert(!ThoiGian::DuThoiGianKhamCapCuu(shift.batDau, 0, phase));
            assert(!ThoiGian::DangTrucCapCuu(shift.ketThuc, phase));
            assert(ThoiGian::TrucCapCuuTiepTheo(shift.batDau - 1, phase) == shift.batDau);
            assert(ThoiGian::TrucCapCuuTiepTheo(shift.batDau + 1, phase) == shift.batDau + 1);
            assert(ThoiGian::TrucCapCuuTiepTheo(shift.ketThuc, phase) == shift.batDau + 2 * 86400);
        }
        assert(ThoiGian::KiemTraLichTruc(emergency));
    }
    // Tat ca 6 nhom cung nhau phu kin 24/7, ke ca cuoi tuan va qua nam moi.
    time_t yearEnd;
    assert(ExamCore::parseTime(std::string("2026-12-29 00:00:00"), yearEnd));
    for (int hour = 0; hour < 10 * 24; ++hour) {
        auto now = yearEnd + hour * 3600;
        int working = 0;
        for (int phase = 0; phase < 6; ++phase) {
            working += ThoiGian::DangTrucCapCuu(now, phase);
            auto shifts = ThoiGian::LichTruc(yearEnd, true, phase, 10);
            bool inShift = false;
            for (const auto& s : shifts) inShift |= s.batDau <= now && now < s.ketThuc;
            assert(inShift == ThoiGian::DangTrucCapCuu(now, phase));
        }
        assert(working == 1);
    }
    const auto day = ThoiGian::DinhDangNgay(monday);
    std::vector<ThoiGian::CaTruc> invalid;
    invalid.push_back({day,monday,monday});
    assert(!ThoiGian::KiemTraLichTruc(invalid));
    invalid[0].ketThuc = monday - 1;
    assert(!ThoiGian::KiemTraLichTruc(invalid));
    invalid[0].ketThuc = monday + 12 * 3600 + 1;
    assert(!ThoiGian::KiemTraLichTruc(invalid));
    invalid[0].ketThuc = monday + 12 * 3600;
    assert(ThoiGian::KiemTraLichTruc(invalid));
    invalid[0].ngay = "invalid";
    assert(!ThoiGian::KiemTraLichTruc(invalid));
    invalid[0] = {day,monday,monday + 3600};
    invalid.push_back({day,monday + 3599,monday + 7200});
    assert(!ThoiGian::KiemTraLichTruc(invalid));
    invalid[1].batDau = monday + 3600;
    std::swap(invalid[0],invalid[1]);
    assert(ThoiGian::KiemTraLichTruc(invalid));
    time_t night;
    assert(ExamCore::parseTime(std::string("2026-10-05 22:00:00"), night));
    invalid[0] = {day,night,night + 4 * 3600};
    invalid[1] = {"2026-10-06",night + 3 * 3600,night + 5 * 3600};
    assert(!ThoiGian::KiemTraLichTruc(invalid));
    // Phan bac si dung cung pha voi API va khong xep luot kham qua gio het ca.
    QuanLyBacSi manager;
    assert(manager.DocCSV("SAP_XEP_BAC_SI/db/bac_si_500_chia_khoa.csv"));
    manager.KhoiTaoLich(monday, true);
    int phase = 0;
    for (int index : manager.LayBacSiTheoKhoa("Khoa Cap cuu")) {
        const auto shifts = ThoiGian::LichTruc(monday + 2 * 86400, true, phase % 6);
        const auto& shift = shifts.front();
        time_t start;
        assert(manager.TinhThoiDiemNhanBenhNhan(index, "Khoa Cap cuu", shift.ketThuc - 20 * 60, 20, start));
        assert(start == shift.ketThuc - 20 * 60);
        assert(manager.TinhThoiDiemNhanBenhNhan(index, "Khoa Cap cuu", shift.ketThuc - 19 * 60, 20, start));
        assert(start == shift.batDau + 2 * 86400);
        assert(!manager.TinhThoiDiemNhanBenhNhan(index, "Khoa Cap cuu", shift.batDau, 481, start));
        ++phase;
    }
    time_t friday;
    assert(ExamCore::parseTime(std::string("2026-10-09 18:00:00"), friday));
    normal = ThoiGian::LichTruc(friday, false, 0, 4);
    assert(normal.size() == 4 && normal[2].ngay == "2026-10-12");
    assert(ThoiGian::LichTruc(monday, false, 0, 0).empty());
}
