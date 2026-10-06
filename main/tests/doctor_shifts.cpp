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
    const auto emergencyA = ThoiGian::LichTruc(monday, true, 0);
    const auto emergencyB = ThoiGian::LichTruc(monday, true, 1);
    assert(emergencyA.size() == 3);
    assert(emergencyB.size() == 3);

    const std::vector<std::string> daysA = {"2026-10-05","2026-10-07","2026-10-09"};
    const std::vector<std::string> daysB = {"2026-10-06","2026-10-08","2026-10-10"};
    for (size_t i = 0; i < emergencyA.size(); ++i) {
        assert(emergencyA[i].ngay == daysA[i]);
        assert(ThoiGian::DinhDang(emergencyA[i].batDau).substr(11) == "06:00:00");
        assert(ThoiGian::DinhDang(emergencyA[i].ketThuc).substr(11) == "18:00:00");
        assert(ThoiGian::DangTrucCapCuu(emergencyA[i].batDau, 0));
        assert(difftime(emergencyA[i].ketThuc, emergencyA[i].batDau) == 12 * 3600);
    }
    for (size_t i = 0; i < emergencyB.size(); ++i) {
        assert(emergencyB[i].ngay == daysB[i]);
        assert(ThoiGian::DangTrucCapCuu(emergencyB[i].batDau, 1));
    }
    assert(!ThoiGian::DangTrucCapCuu(emergencyA[0].batDau, 1));
    assert(!ThoiGian::DangTrucCapCuu(emergencyB[0].batDau, 0));
    assert(ThoiGian::DuThoiGianKhamCapCuu(emergencyA[0].ketThuc - 20 * 60, 20, 0));
    assert(!ThoiGian::DuThoiGianKhamCapCuu(emergencyA[0].ketThuc - 19 * 60, 20, 0));
    assert(ThoiGian::DuThoiGianKhamCapCuu(emergencyA[0].batDau, 720, 0));
    assert(!ThoiGian::DuThoiGianKhamCapCuu(emergencyA[0].batDau, 721, 0));
    assert(ThoiGian::TrucCapCuuTiepTheo(emergencyA[0].ketThuc, 0) == emergencyA[1].batDau);
    assert(ThoiGian::TrucCapCuuTiepTheo(emergencyB[0].ketThuc, 1) == emergencyB[1].batDau);
    assert(ThoiGian::KiemTraLichTruc(emergencyA));
    assert(ThoiGian::KiemTraLichTruc(emergencyB));
    const auto nightsA = ThoiGian::LichTruc(monday, true, 2);
    const auto nightsB = ThoiGian::LichTruc(monday, true, 3);
    assert(nightsA.size() == 3 && nightsB.size() == 3);
    for (const auto& shift : nightsA) {
        assert(ThoiGian::DinhDang(shift.batDau).substr(11) == "18:00:00");
        assert(ThoiGian::DinhDang(shift.ketThuc).substr(11) == "06:00:00");
        assert(difftime(shift.ketThuc, shift.batDau) == 12 * 3600);
        assert(ThoiGian::DangTrucCapCuu(shift.batDau, 2));
        assert(ThoiGian::DangTrucCapCuu(shift.ketThuc - 1, 2));
        assert(!ThoiGian::DangTrucCapCuu(shift.ketThuc, 2));
        assert(ThoiGian::DuThoiGianKhamCapCuu(shift.ketThuc - 20 * 60, 20, 2));
        assert(!ThoiGian::DuThoiGianKhamCapCuu(shift.ketThuc - 19 * 60, 20, 2));
    }
    assert(ThoiGian::KiemTraLichTruc(nightsA));
    assert(ThoiGian::KiemTraLichTruc(nightsB));
    time_t afterMidnight;
    assert(ExamCore::parseTime(std::string("2026-10-06 02:00:00"), afterMidnight));
    assert(ThoiGian::DangTrucCapCuu(afterMidnight, 2));
    assert(!ThoiGian::DangTrucCapCuu(afterMidnight, 3));
    assert(ThoiGian::TrucCapCuuTiepTheo(afterMidnight, 2) == afterMidnight);
    assert(ThoiGian::LichTruc(afterMidnight, true, 2).front().batDau == nightsA.front().batDau);
    time_t monthBoundary;
    assert(ExamCore::parseTime(std::string("2026-11-01 02:00:00"), monthBoundary));
    assert(ThoiGian::DangTrucCapCuu(monthBoundary, 3));
    assert(ThoiGian::DinhDang(ThoiGian::LichTruc(monthBoundary, true, 3).front().batDau) == "2026-10-31 18:00:00");

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
    // Phân bác sĩ dùng cùng hai ca 12 giờ và không xếp khám quá giờ hết ca.
    QuanLyBacSi manager;
    assert(manager.DocCSV("SAP_XEP_BAC_SI/db/bac_si_500_chia_khoa.csv"));
    manager.KhoiTaoLich(monday, true);
    int phase = 0;
    for (int index : manager.LayBacSiTheoKhoa("Khoa Cap cuu")) {
        const auto shifts = ThoiGian::LichTruc(monday, true, phase % 4);
        assert(manager.LayPhaTrucCapCuu(index) == phase % 4);
        const auto& shift = shifts.front();
        time_t start;
        assert(manager.TinhThoiDiemNhanBenhNhan(index, "Khoa Cap cuu", shift.ketThuc - 20 * 60, 20, start));
        assert(start == shift.ketThuc - 20 * 60);
        assert(manager.TinhThoiDiemNhanBenhNhan(index, "Khoa Cap cuu", shift.ketThuc - 19 * 60, 20, start));
        assert(start == shifts[1].batDau);
        assert(!manager.TinhThoiDiemNhanBenhNhan(index, "Khoa Cap cuu", shift.batDau, 721, start));
        ++phase;
    }
    time_t friday;
    assert(ExamCore::parseTime(std::string("2026-10-09 18:00:00"), friday));
    normal = ThoiGian::LichTruc(friday, false, 0, 4);
    assert(normal.size() == 4 && normal[2].ngay == "2026-10-12");
    assert(ThoiGian::LichTruc(monday, false, 0, 0).empty());
}
