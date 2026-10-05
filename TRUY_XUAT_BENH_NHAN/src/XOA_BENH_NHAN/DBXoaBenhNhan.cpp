#include "XoaBenhNhan.h"
#include "../TRUY_XUAT/TruyXuat.h"
#include "MangDongBenhNhan.h"

#include <iostream>
#include <sqlite3.h>
#include <string>

using namespace std;

namespace {

constexpr const char* priorityPath = "THAY_DOI_MUC_DO_UU_TIEN/db/priority.db";

bool executeSql(sqlite3* database, const char* sql, const char* operation) {
    char* errorMessage = nullptr;
    if (sqlite3_exec(database, sql, nullptr, nullptr, &errorMessage) == SQLITE_OK) {
        return true;
    }
    cerr << operation << ": "
         << (errorMessage ? errorMessage : sqlite3_errmsg(database)) << '\n';
    sqlite3_free(errorMessage);
    return false;
}

bool ghiDanhSachPriorityDB(const MangDongBenhNhan& records) {
    sqlite3* database = nullptr;
    if (sqlite3_open_v2(priorityPath, &database, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        cerr << "Khong mo duoc priority.db: "
             << (database ? sqlite3_errmsg(database) : "loi SQLite") << '\n';
        if (database) sqlite3_close(database);
        return false;
    }

    sqlite3_busy_timeout(database, 5000);
    if (!executeSql(database, "BEGIN IMMEDIATE;", "Loi bat dau cap nhat priority.db")) {
        sqlite3_close(database);
        return false;
    }

    bool success = executeSql(database, "DELETE FROM priority_checkins;", "Loi xoa du lieu cu");
    sqlite3_stmt* statement = nullptr;
    const char* insertSql = R"(
        INSERT INTO priority_checkins
            (checkin_id, patient_id, department, checkin_time,
             base_priority, current_priority, last_update)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )";
    if (success && sqlite3_prepare_v2(database, insertSql, -1, &statement, nullptr) != SQLITE_OK) {
        cerr << "Loi chuan bi ghi priority.db: " << sqlite3_errmsg(database) << '\n';
        success = false;
    }

    for (int index = 0; success && index < records.size(); ++index) {
        const auto& record = records[index];
        sqlite3_bind_int(statement, 1, record.checkinId);
        sqlite3_bind_int(statement, 2, record.patientId);
        sqlite3_bind_text(statement, 3, record.department.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 4, record.checkinTime.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(statement, 5, record.basePriority);
        sqlite3_bind_int(statement, 6, record.currentPriority);
        if (record.lastUpdate.empty())
            sqlite3_bind_null(statement, 7);
        else
            sqlite3_bind_text(statement, 7, record.lastUpdate.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(statement) != SQLITE_DONE) {
            cerr << "Loi ghi priority.db: " << sqlite3_errmsg(database) << '\n';
            success = false;
            break;
        }
        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);
    }

    sqlite3_finalize(statement);
    if (success)
        success = executeSql(database, "COMMIT;", "Loi hoan tat cap nhat priority.db");
    if (!success)
        executeSql(database, "ROLLBACK;", "Loi rollback priority.db");
    sqlite3_close(database);
    return success;
}

} // end namespace

bool DBXoaBenhNhan::xoaBenhNhan(int patientId) {
    if (patientId <= 0) return false;

    // 1. KÉO DỮ LIỆU LÊN RAM (Tầng DSA Core)
    MangDongBenhNhan records;
    if (!DBTruyXuat::docDanhSachBenhNhan(records)) {
        cerr << "Khong the doc CSDL de xoa.\n";
        return false;
    }

    // 2. THỰC THI THUẬT TOÁN TÌM VÀ XÓA TRÊN RAM (Không dùng WHERE của SQL)
    bool found = false;
    for (int i = 0; i < records.size(); ++i) {
        if (records[i].patientId == patientId) {
            records.erase(i); // O(n) shifts[cite: 33]
            found = true;
            break;
        }
    }

    if (!found) {
        cout << "Khong tim thay benh nhan ID " << patientId << " de xoa.\n";
        return false;
    }

    MangDongBenhNhan buffer;
    for (int i = 0; i < records.size(); ++i)
        buffer.push_back(records[i]);
    if (!records.empty())
        ThuatToanSapXep::sapXepTron(records, buffer, 0, records.size() - 1);

    if (!ghiDanhSachPriorityDB(records)) return false;
    const bool success = DBTruyXuat::ghiDanhSachDaSapXep(records);
    if (success) {
        cout << "Da xoa benh nhan ID " << patientId << " thanh cong.\n";
    } else {
        cerr << "Da cap nhat priority.db nhung khong cap nhat duoc hang doi truy xuat.\n";
    }
    return success;
}

bool DBXoaBenhNhan::xoaTatCaBenhNhan() {
    // 1. Tạo mảng rỗng trên RAM
    MangDongBenhNhan emptyRecords;

    // 2. Xóa trắng hai database: priority là nguồn, truyXuat là hàng đợi dẫn xuất.
    const bool p_success = ghiDanhSachPriorityDB(emptyRecords);
    const bool r_success = DBTruyXuat::ghiDanhSachDaSapXep(emptyRecords);

    if (p_success && r_success) {
        cout << "Da xoa toan bo benh nhan.\n";
        return true;
    }
    return false;
}