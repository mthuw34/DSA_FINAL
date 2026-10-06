#include "../src/THAY_DOI_TU_DONG/AutoPriority.h"
#include "../src/CAP_NHAT_THU_CONG/PriorityManager.h"
#include "../src/LUU_MUC_UU_TIEN/PriorityStorage.h"
#include "../../TRUY_XUAT_BENH_NHAN/src/TRUY_XUAT/TruyXuat.h"
#include <cassert>
#include <vector>

int main() {
    // Different departments, original levels and manual changes must not beat FIFO.
    HoSoTruyXuat earlier{}, later{}, urgent{};
    earlier.checkinId = 30;
    earlier.departmentOrder = 10;
    earlier.currentPriority = 2;
    earlier.basePriority = 5;
    earlier.checkinTime = "2026-10-05 23:59:59";
    earlier.lastUpdate = "2026-10-06 10:00:00";
    later = earlier;
    later.checkinId = 10;
    later.departmentOrder = 1;
    later.basePriority = 2;
    later.isTroNangLamSang = true;
    later.checkinTime = "2026-10-06 00:00:00";
    later.lastUpdate = "2026-10-06 09:00:00";
    urgent = later;
    urgent.checkinId = 40;
    urgent.currentPriority = 1;

    MangDongBenhNhan records, buffer;
    for (const auto& record : {later, earlier, urgent}) {
        records.push_back(record);
        buffer.push_back(record);
    }
    ThuatToanSapXep::sapXepTron(records, buffer, 0, records.size() - 1);
    assert(records[0].checkinId == 40);
    assert(records[1].checkinId == 30);
    assert(records[2].checkinId == 10);
    later.checkinTime = earlier.checkinTime;
    assert(ThuatToanSapXep::xetUuTien(later, earlier));
    assert(!ThuatToanSapXep::xetUuTien(earlier, earlier));

    sqlite3* db = nullptr;
    assert(sqlite3_open(":memory:", &db) == SQLITE_OK);
    assert(sqlite3_exec(db, "CREATE TABLE priority_checkins (checkin_id INTEGER PRIMARY KEY, "
        "patient_id INTEGER, department TEXT, checkin_time TEXT, base_priority INTEGER, "
        "current_priority INTEGER, last_update TEXT);", nullptr, nullptr, nullptr) == SQLITE_OK);
    for (int level = 1; level <= 5; ++level) {
        const auto sql = "INSERT INTO priority_checkins VALUES (" + std::to_string(level) + "," +
            std::to_string(level) + ",'Khoa Cap cuu','2000-01-03 08:00:00'," +
            std::to_string(level) + "," + std::to_string(level) + ",'2000-01-03 08:00:00');";
        assert(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK);
    }
    AutoPriorityHeap heap;
    loadPatients(db, heap);
    assert(heap.getSize() == 3);
    // Also reject stale heap entries for patients already at level 1 or 2.
    heap.insert({1, 0});
    heap.insert({2, 0});
    processAuto(db, heap);
    PriorityManager manager(db);
    for (int id = 1; id <= 5; ++id) {
        int base = 0, current = 0;
        assert(manager.getPriority(id, base, current));
        assert(base == id);
        assert(current == (id == 1 ? 1 : 2));
    }
    assert(manager.updatePriority(5, 1));
    AutoPriorityHeap next;
    loadPatients(db, next);
    assert(next.isEmpty());
    processAuto(db, next);
    int base = 0, current = 0;
    assert(manager.getPriority(5, base, current) && current == 1);
    sqlite3_close(db);
}
