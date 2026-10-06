#include "TaoBangTruyXuat.h"
#include "../TRUY_XUAT/TruyXuat.h"

#include <iostream>
#include <sqlite3.h>
#include <string>

using namespace std;

namespace {

constexpr const char* retrievalPath = "TRUY_XUAT_BENH_NHAN/db/truyXuat.db";

bool executeSql(sqlite3* database, const string& sql, const char* operation) 
    {
        char* errorMessage = nullptr;
        if (sqlite3_exec(database, sql.c_str(), nullptr, nullptr, &errorMessage) == SQLITE_OK) {
            return true;
        }

        cerr << operation << ": "
                << (errorMessage ? errorMessage : sqlite3_errmsg(database)) << '\n';
        sqlite3_free(errorMessage);
        return false;
    }

}

bool DBTaoBang::taoBangTruyXuat() 
{
    sqlite3* database = nullptr;
    if (sqlite3_open_v2(
            retrievalPath,
            &database,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
            nullptr
        ) != SQLITE_OK) 
        {
            cerr << "Khong mo duoc truyXuat.db: "
                    << (database ? sqlite3_errmsg(database) : "loi SQLite") << '\n';
            if (database) sqlite3_close(database);
            return false;
        }

    if (!executeSql(database, "BEGIN TRANSACTION;", "Khong the bat dau giao dich tao bang")) {
        sqlite3_close(database);
        return false;
    }

    bool success = true;
    for (const auto& department : CauHinhTruyXuat::danhSachKhoa) {
        const string createSql =
            "CREATE TABLE IF NOT EXISTS " + string(department.tenBang) + R"( (
                retrieval_order INTEGER PRIMARY KEY,
                checkin_id INTEGER NOT NULL UNIQUE,
                patient_id INTEGER NOT NULL,
                department TEXT NOT NULL,
                checkin_time TEXT NOT NULL,
                base_priority INTEGER NOT NULL,
                current_priority INTEGER NOT NULL,
                last_update TEXT
            );)";
        if (!executeSql(database, createSql, "Loi tao bang truy xuat")) {
            success = false;
            break;
        }
    }

    if (success && executeSql(database, "COMMIT;", "Khong the hoan tat giao dich tao bang")) {
        sqlite3_close(database);
        cout << "Tao bang truy xuat thanh cong.\n";
        return true;
    }

    executeSql(database, "ROLLBACK;", "Khong the rollback giao dich tao bang");
    sqlite3_close(database);
    return false;
}
