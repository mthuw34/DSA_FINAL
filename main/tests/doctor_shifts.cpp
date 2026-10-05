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
    const auto emergency = ThoiGian::LichTruc(monday, true, 0);
    assert(emergency.size() == 7);
    for (const auto& shift : emergency) {
        assert(ThoiGian::DinhDang(shift.batDau).substr(11) == "07:00:00");
        assert(ThoiGian::DinhDang(shift.ketThuc).substr(11) == "17:00:00");
        assert(ThoiGian::DangTrucCapCuu(shift.batDau, 0));
        assert(difftime(shift.ketThuc, shift.batDau) == 10 * 3600);
        assert(ThoiGian::DuThoiGianKhamCapCuu(shift.ketThuc - 20 * 60, 20, 0));
        assert(!ThoiGian::DuThoiGianKhamCapCuu(shift.ketThuc - 19 * 60, 20, 0));
        assert(!ThoiGian::DuThoiGianKhamCapCuu(shift.batDau, 601, 0));
        assert(!ThoiGian::DangTrucCapCuu(shift.ketThuc, 0));
        assert(ThoiGian::TrucCapCuuTiepTheo(shift.batDau - 1, 0) == shift.batDau);
        assert(ThoiGian::TrucCapCuuTiepTheo(shift.ketThuc, 0) == shift.batDau + 86400);
    }
    assert(ThoiGian::KiemTraLichTruc(emergency));

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
    // Phân bác sĩ dùng cùng ca 07:00-17:00 và không xếp lượt khám quá giờ hết ca.
    QuanLyBacSi manager;
    assert(manager.DocCSV("SAP_XEP_BAC_SI/db/bac_si_500_chia_khoa.csv"));
    manager.KhoiTaoLich(monday, true);
    int phase = 0;
    for (int index : manager.LayBacSiTheoKhoa("Khoa Cap cuu")) {
        const auto shifts = ThoiGian::LichTruc(monday + 2 * 86400, true, 0);
        const auto& shift = shifts.front();
        time_t start;
        assert(manager.TinhThoiDiemNhanBenhNhan(index, "Khoa Cap cuu", shift.ketThuc - 20 * 60, 20, start));
        assert(start == shift.ketThuc - 20 * 60);
        assert(manager.TinhThoiDiemNhanBenhNhan(index, "Khoa Cap cuu", shift.ketThuc - 19 * 60, 20, start));
        assert(start == shift.batDau + 86400);
        assert(!manager.TinhThoiDiemNhanBenhNhan(index, "Khoa Cap cuu", shift.batDau, 601, start));
        ++phase;
    }
    time_t friday;
    assert(ExamCore::parseTime(std::string("2026-10-09 18:00:00"), friday));
    normal = ThoiGian::LichTruc(friday, false, 0, 4);
    assert(normal.size() == 4 && normal[2].ngay == "2026-10-12");
    assert(ThoiGian::LichTruc(monday, false, 0, 0).empty());
}
