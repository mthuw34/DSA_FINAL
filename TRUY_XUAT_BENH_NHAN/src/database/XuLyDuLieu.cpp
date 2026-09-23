#include "XuLyDuLieu.h"
#include "ThuatToanSapXep.h"
#include <cstdio>

using namespace std;

bool XuLyDuLieu::layDanhSachBenhNhan(
    sqlite3* priorityDatabase,
    vector<HoSoTruyXuat>& outRecords
) {
    const char* query = R"(
        SELECT checkin_id, patient_id, department, checkin_time,
               base_priority, current_priority, last_update,
               CASE department
                   WHEN 'Khoa Cap cuu' THEN 1 WHEN 'Khoa Noi' THEN 2
                   WHEN 'Khoa Ngoai' THEN 3 WHEN 'Khoa Tim mach' THEN 4
                   WHEN 'Khoa Nhi' THEN 5 WHEN 'Khoa San' THEN 6
                   WHEN 'Khoa Tai Mui Hong' THEN 7 WHEN 'Khoa Mat' THEN 8
                   WHEN 'Khoa Da lieu' THEN 9 WHEN 'Khoa Than kinh' THEN 10
                   ELSE 0
               END
        FROM priority_checkins;
    )";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(priorityDatabase, query, -1, &statement, nullptr) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return false;
    }

    int result = SQLITE_ROW;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        HoSoTruyXuat record;
        record.checkinId = sqlite3_column_int(statement, 0);
        record.patientId = sqlite3_column_int(statement, 1);
        record.department = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
        record.departmentOrder = sqlite3_column_int(statement, 7);

        if (record.departmentOrder == 0) {
            record.departmentOrder = ThuatToanSapXep::layThuTuKhoa(record.department);
        }

        const unsigned char* checkinTimeText = sqlite3_column_text(statement, 3);
        record.checkinTime = checkinTimeText ? reinterpret_cast<const char*>(checkinTimeText) : "";

        record.basePriority = sqlite3_column_int(statement, 4);
        record.currentPriority = sqlite3_column_int(statement, 5);
        const unsigned char* lastUpdateText = sqlite3_column_text(statement, 6);
        record.lastUpdate = lastUpdateText ? reinterpret_cast<const char*>(lastUpdateText) : "";
        record.priorityChanged = record.basePriority != record.currentPriority;
        record.priority = record.currentPriority;
        outRecords.push_back(record);
    }

    bool success = (result == SQLITE_DONE);
    sqlite3_finalize(statement);
    return success;
}

bool XuLyDuLieu::xuatDuLieuDaSapXep(const vector<HoSoTruyXuat>& sortedRecords, const char* outputPath) {
    sqlite3* outputDatabase = nullptr;

    if (sqlite3_open_v2(
            outputPath,
            &outputDatabase,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
            nullptr
        ) != SQLITE_OK) {
        if (outputDatabase != nullptr) {
            sqlite3_close(outputDatabase);
        }
        return false;
    }

    const char* tableNames[] = {
        "queue_khoa_cap_cuu", "queue_khoa_noi", "queue_khoa_ngoai",
        "queue_khoa_tim_mach", "queue_khoa_nhi", "queue_khoa_san",
        "queue_khoa_tai_mui_hong", "queue_khoa_mat", "queue_khoa_da_lieu",
        "queue_khoa_than_kinh"
    };

    const char* dropForeignTablesSql = R"(
        PRAGMA foreign_keys = ON;
        DROP TABLE IF EXISTS retrieval_queue;
        DROP TABLE IF EXISTS departments;
    )";

    char* errorMessage = nullptr;
    if (sqlite3_exec(outputDatabase, dropForeignTablesSql, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        sqlite3_free(errorMessage);
        sqlite3_close(outputDatabase);
        return false;
    }

    for (const char* tableName : tableNames) {
        char dropTableSql[256];
        snprintf(dropTableSql, sizeof(dropTableSql), "DROP TABLE IF EXISTS %s;", tableName);
        if (sqlite3_exec(outputDatabase, dropTableSql, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
            sqlite3_free(errorMessage);
            sqlite3_close(outputDatabase);
            return false;
        }
    }

    const char* createTableFormat = R"(
        CREATE TABLE %s (
            retrieval_order INTEGER PRIMARY KEY,
            checkin_id INTEGER NOT NULL UNIQUE,
            patient_id INTEGER NOT NULL,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            base_priority INTEGER NOT NULL,
            current_priority INTEGER NOT NULL,
            last_update TEXT
        );
    )";

    for (const char* tableName : tableNames) {
        char createTableSql[1024];
        snprintf(createTableSql, sizeof(createTableSql), createTableFormat, tableName);
        if (sqlite3_exec(outputDatabase, createTableSql, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
            sqlite3_free(errorMessage);
            sqlite3_close(outputDatabase);
            return false;
        }
    }

    const char* insertFormat = "INSERT INTO %s VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* departmentStatements[10] = {};
    for (int index = 0; index < 10; ++index) {
        char insertSql[256];
        snprintf(insertSql, sizeof(insertSql), insertFormat, tableNames[index]);
        if (sqlite3_prepare_v2(outputDatabase, insertSql, -1, &departmentStatements[index], nullptr) != SQLITE_OK) {
            for (sqlite3_stmt* statement : departmentStatements) {
                if (statement) sqlite3_finalize(statement);
            }
            sqlite3_close(outputDatabase);
            return false;
        }
    }

    bool success = true;
    int departmentOrders[10] = {};
    for (const HoSoTruyXuat& record : sortedRecords) {
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
            success = false;
            break;
        }
    }

    for (sqlite3_stmt* statement : departmentStatements) {
        if (statement) sqlite3_finalize(statement);
    }
    sqlite3_close(outputDatabase);
    return success;
}