#include "ThoiGian.h"
#include "ExamCore.h"
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
    for (int phase : {0, 1}) {
        const auto emergency = ThoiGian::LichTruc(monday, true, phase);
        assert(emergency.size() == 3 || emergency.size() == 4);
        for (const auto& shift : emergency) {
            assert(ThoiGian::DangTrucCapCuu(shift.batDau, phase));
            assert(ThoiGian::DinhDang(shift.batDau).substr(11) == "00:00:00");
            assert(ThoiGian::DinhDang(shift.ketThuc).substr(11) == "00:00:00");
            assert(shift.ngay == ThoiGian::DinhDangNgay(shift.batDau));
            assert(shift.ketThuc > shift.batDau);
            assert(!ThoiGian::DangTrucCapCuu(shift.ketThuc, phase));
        }
    }
    time_t friday;
    assert(ExamCore::parseTime(std::string("2026-10-09 18:00:00"), friday));
    normal = ThoiGian::LichTruc(friday, false, 0, 4);
    assert(normal.size() == 4 && normal[2].ngay == "2026-10-12");
    assert(ThoiGian::LichTruc(monday, false, 0, 0).empty());
}
