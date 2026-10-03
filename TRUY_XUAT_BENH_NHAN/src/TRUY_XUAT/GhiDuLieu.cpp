#include "TruyXuat.h"

#include <iostream>
#include <sqlite3.h>
#include <string>

using namespace std;

bool GhiDuLieu::ghiDanhSachDaSapXep(
    const vector<HoSoTruyXuat>& sortedRecords,
    const char* outputPath
) {
    sqlite3* outputDatabase = nullptr;

    if (sqlite3_open_v2(
            outputPath,
            &outputDatabase,
            SQLITE_OPEN_READWRITE,
            nullptr
        ) != SQLITE_OK) {
        cerr << "Khong mo duoc truyXuat.db: "
             << (outputDatabase ? sqlite3_errmsg(outputDatabase) : "loi SQLite") << '\n';
        if (outputDatabase != nullptr) {
            sqlite3_close(outputDatabase);
        }
        return false;
    }

    static const char* tableNames[] = {
        "queue_khoa_cap_cuu",
        "queue_khoa_noi",
        "queue_khoa_ngoai",
        "queue_khoa_tim_mach",
        "queue_khoa_nhi",
        "queue_khoa_san",
        "queue_khoa_tai_mui_hong",
        "queue_khoa_mat",
        "queue_khoa_da_lieu",
        "queue_khoa_than_kinh"
    };

    char* errorMessage = nullptr;
    if (sqlite3_exec(outputDatabase, "BEGIN TRANSACTION;", nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        cerr << "Khong the bat dau giao dich ghi du lieu: "
             << (errorMessage ? errorMessage : sqlite3_errmsg(outputDatabase)) << '\n';
        sqlite3_free(errorMessage);
        sqlite3_close(outputDatabase);
        return false;
    }

    bool success = true;
    for (const char* tableName : tableNames) {
        const string clearTableSql = string("DELETE FROM ") + tableName + ";";
        if (sqlite3_exec(outputDatabase, clearTableSql.c_str(), nullptr, nullptr, &errorMessage) != SQLITE_OK) {
            cerr << "Khong the xoa du lieu cu trong " << tableName << ": "
                 << (errorMessage ? errorMessage : sqlite3_errmsg(outputDatabase)) << '\n';
            sqlite3_free(errorMessage);
            errorMessage = nullptr;
            success = false;
            break;
        }
    }

    sqlite3_stmt* departmentStatements[10] = {};
    for (int index = 0; success && index < 10; ++index) {
        const string insertSql = string("INSERT INTO ") + tableNames[index]
            + " VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
        if (sqlite3_prepare_v2(outputDatabase, insertSql.c_str(), -1, &departmentStatements[index], nullptr) != SQLITE_OK) {
            cerr << "Khong the chuan bi cau lenh ghi vao " << tableNames[index]
                 << ": " << sqlite3_errmsg(outputDatabase) << '\n';
            success = false;
        }
    }

    int departmentOrders[10] = {};
    for (const HoSoTruyXuat& record : sortedRecords) {
        if (!success) break;
        if (record.departmentOrder < 1 || record.departmentOrder > 10) continue;

        sqlite3_stmt* departmentStatement = departmentStatements[record.departmentOrder - 1];
        sqlite3_reset(departmentStatement);
        sqlite3_clear_bindings(departmentStatement);
        sqlite3_bind_int(departmentStatement, 1, ++departmentOrders[record.departmentOrder - 1]);
        sqlite3_bind_int(departmentStatement, 2, record.checkinId);
        sqlite3_bind_int(departmentStatement, 3, record.patientId);
        sqlite3_bind_text(departmentStatement, 4, record.department.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(departmentStatement, 5, record.checkinTime.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(departmentStatement, 6, record.basePriority);
        sqlite3_bind_int(departmentStatement, 7, record.currentPriority);
        if (record.lastUpdate.empty()) {
            sqlite3_bind_null(departmentStatement, 8);
        } else {
            sqlite3_bind_text(departmentStatement, 8, record.lastUpdate.c_str(), -1, SQLITE_TRANSIENT);
        }

        if (sqlite3_step(departmentStatement) != SQLITE_DONE) {
            cerr << "Loi ghi du lieu vao " << tableNames[record.departmentOrder - 1]
                 << ": " << sqlite3_errmsg(outputDatabase) << '\n';
            success = false;
        }
    }

    for (sqlite3_stmt* statement : departmentStatements) {
        if (statement) sqlite3_finalize(statement);
    }

    if (success && sqlite3_exec(outputDatabase, "COMMIT;", nullptr, nullptr, &errorMessage) == SQLITE_OK) {
        sqlite3_close(outputDatabase);
        return true;
    }

    if (success) {
        cerr << "Khong the hoan tat giao dich ghi du lieu: "
             << (errorMessage ? errorMessage : sqlite3_errmsg(outputDatabase)) << '\n';
    }
    sqlite3_free(errorMessage);
    sqlite3_exec(outputDatabase, "ROLLBACK;", nullptr, nullptr, nullptr);
    sqlite3_close(outputDatabase);
    return false;
}
