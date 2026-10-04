#include "TruyXuat.h"

#include <array>
#include <iostream>
#include <sqlite3.h>
#include <string>

namespace {

constexpr const char* priorityPath = "THAY_DOI_MUC_DO_UU_TIEN/db/priority.db";
constexpr const char* retrievalPath = "TRUY_XUAT_BENH_NHAN/db/truyXuat.db";

void reportError(sqlite3* database, const char* operation) {
    std::cerr << operation << ": "
              << (database ? sqlite3_errmsg(database) : "loi SQLite") << '\n';
}

bool executeSql(sqlite3* database, const char* sql, const char* operation) {
    char* errorMessage = nullptr;
    if (sqlite3_exec(database, sql, nullptr, nullptr, &errorMessage) == SQLITE_OK) {
        return true;
    }

    std::cerr << operation << ": "
              << (errorMessage ? errorMessage : sqlite3_errmsg(database)) << '\n';
    sqlite3_free(errorMessage);
    return false;
}

}

bool DBTruyXuat::docDanhSachBenhNhan(std::vector<HoSoTruyXuat>& outRecords) {
    outRecords.clear();

    sqlite3* database = nullptr;
    if (sqlite3_open_v2(priorityPath, &database, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        reportError(database, "Khong mo duoc priority.db");
        if (database) sqlite3_close(database);
        return false;
    }

    constexpr const char* query = R"(
        SELECT checkin_id, patient_id, department, checkin_time,
               base_priority, current_priority, last_update
        FROM priority_checkins;
    )";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, query, -1, &statement, nullptr) != SQLITE_OK) {
        reportError(database, "Khong the chuan bi truy van priority.db");
        sqlite3_close(database);
        return false;
    }

    int result = SQLITE_ROW;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        HoSoTruyXuat record{};
        record.checkinId = sqlite3_column_int(statement, 0);
        record.patientId = sqlite3_column_int(statement, 1);

        const auto* departmentText = sqlite3_column_text(statement, 2);
        record.department = departmentText
            ? reinterpret_cast<const char*>(departmentText)
            : "";
        record.departmentOrder = CauHinhTruyXuat::thuTuKhoa(record.department);

        const auto* checkinTimeText = sqlite3_column_text(statement, 3);
        record.checkinTime = checkinTimeText
            ? reinterpret_cast<const char*>(checkinTimeText)
            : "";
        record.basePriority = sqlite3_column_int(statement, 4);
        record.currentPriority = sqlite3_column_int(statement, 5);

        const auto* lastUpdateText = sqlite3_column_text(statement, 6);
        record.lastUpdate = lastUpdateText
            ? reinterpret_cast<const char*>(lastUpdateText)
            : "";
        outRecords.push_back(record);
    }

    const bool success = result == SQLITE_DONE;
    if (!success) {
        reportError(database, "Loi doc priority.db");
        outRecords.clear();
    }

    sqlite3_finalize(statement);
    sqlite3_close(database);
    return success;
}

bool DBTruyXuat::ghiDanhSachDaSapXep(
    const std::vector<HoSoTruyXuat>& sortedRecords
) {
    sqlite3* database = nullptr;
    if (sqlite3_open_v2(retrievalPath, &database, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        reportError(database, "Khong mo duoc truyXuat.db");
        if (database) sqlite3_close(database);
        return false;
    }

    if (!executeSql(database, "BEGIN TRANSACTION;", "Khong the bat dau giao dich ghi")) {
        sqlite3_close(database);
        return false;
    }

    bool success = true;
    for (const auto& department : CauHinhTruyXuat::danhSachKhoa) {
        const std::string clearSql =
            "DELETE FROM " + std::string(department.tenBang) + ";";
        if (!executeSql(database, clearSql.c_str(), "Khong the xoa du lieu hang doi cu")) {
            success = false;
            break;
        }
    }

    std::array<sqlite3_stmt*, CauHinhTruyXuat::danhSachKhoa.size()> statements{};
    for (std::size_t index = 0; success && index < statements.size(); ++index) {
        const std::string insertSql =
            "INSERT INTO " + std::string(CauHinhTruyXuat::danhSachKhoa[index].tenBang)
            + " VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
        if (sqlite3_prepare_v2(
                database,
                insertSql.c_str(),
                -1,
                &statements[index],
                nullptr
            ) != SQLITE_OK) {
            reportError(database, "Khong the chuan bi cau lenh ghi hang doi");
            success = false;
        }
    }

    std::array<int, CauHinhTruyXuat::danhSachKhoa.size()> departmentOrders{};
    for (const HoSoTruyXuat& record : sortedRecords) {
        if (!success) break;
        if (record.departmentOrder < 1
            || static_cast<std::size_t>(record.departmentOrder) > statements.size()) {
            continue;
        }

        const std::size_t departmentIndex =
            static_cast<std::size_t>(record.departmentOrder - 1);
        sqlite3_stmt* statement = statements[departmentIndex];
        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);
        sqlite3_bind_int(statement, 1, ++departmentOrders[departmentIndex]);
        sqlite3_bind_int(statement, 2, record.checkinId);
        sqlite3_bind_int(statement, 3, record.patientId);
        sqlite3_bind_text(statement, 4, record.department.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 5, record.checkinTime.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(statement, 6, record.basePriority);
        sqlite3_bind_int(statement, 7, record.currentPriority);
        if (record.lastUpdate.empty()) {
            sqlite3_bind_null(statement, 8);
        } else {
            sqlite3_bind_text(statement, 8, record.lastUpdate.c_str(), -1, SQLITE_TRANSIENT);
        }

        if (sqlite3_step(statement) != SQLITE_DONE) {
            reportError(database, "Loi ghi du lieu hang doi");
            success = false;
        }
    }

    for (sqlite3_stmt* statement : statements) {
        if (statement) sqlite3_finalize(statement);
    }

    if (success && executeSql(database, "COMMIT;", "Khong the hoan tat giao dich ghi")) {
        sqlite3_close(database);
        return true;
    }

    executeSql(database, "ROLLBACK;", "Khong the rollback giao dich ghi");
    sqlite3_close(database);
    return false;
}
