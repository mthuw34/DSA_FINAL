#include "XoaBenhNhan.h"
#include "../TRUY_XUAT/TruyXuat.h"

#include <filesystem>
#include <iostream>
#include <sqlite3.h>
#include <string>

using namespace std;

namespace {

constexpr const char* priorityPath = "THAY_DOI_MUC_DO_UU_TIEN/db/priority.db";
constexpr const char* retrievalPath = "TRUY_XUAT_BENH_NHAN/db/truyXuat.db";

void reportError(sqlite3* database, const char* operation) {
    cerr << operation << ": "
              << (database ? sqlite3_errmsg(database) : "loi SQLite") << '\n';
}

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

}

bool DBXoaBenhNhan::xoaBenhNhan(int patientId) {
    // Precondition: patientId must be positive and both database files must already exist.
    if (patientId <= 0) {
        cerr << "ID benh nhan phai lon hon 0.\n";
        return false;
    }
    if (!filesystem::exists(priorityPath) || !filesystem::exists(retrievalPath)) {
        cerr << "Khong tim thay priority.db hoac truyXuat.db. "
                     "Hay chay chuong trinh tu thu muc goc workspace.\n";
        return false;
    }

    sqlite3* database = nullptr;
    if (sqlite3_open_v2(priorityPath, &database, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        reportError(database, "Khong mo duoc priority.db");
        if (database) sqlite3_close(database);
        return false;
    }

    sqlite3_stmt* attachStatement = nullptr;
    if (sqlite3_prepare_v2(
            database,
            "ATTACH DATABASE ? AS retrieval;",
            -1,
            &attachStatement,
            nullptr
        ) != SQLITE_OK) {
        reportError(database, "Khong the chuan bi ket noi truyXuat.db");
        sqlite3_close(database);
        return false;
    }
    sqlite3_bind_text(attachStatement, 1, retrievalPath, -1, SQLITE_TRANSIENT);
    const int attachResult = sqlite3_step(attachStatement);
    sqlite3_finalize(attachStatement);
    if (attachResult != SQLITE_DONE) {
        reportError(database, "Khong the ket noi truyXuat.db");
        sqlite3_close(database);
        return false;
    }

    if (!executeSql(database, "BEGIN IMMEDIATE;", "Khong the bat dau giao dich xoa")) {
        sqlite3_exec(database, "DETACH DATABASE retrieval;", nullptr, nullptr, nullptr);
        sqlite3_close(database);
        return false;
    }

    int deletedRows = 0;
    bool success = true;
    const auto deleteForPatient = [&](const string& sql) {
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(database, sql.c_str(), -1, &statement, nullptr) != SQLITE_OK) {
            reportError(database, "Khong the chuan bi cau lenh xoa");
            return false;
        }

        sqlite3_bind_int(statement, 1, patientId);
        const int result = sqlite3_step(statement);
        if (result != SQLITE_DONE) {
            reportError(database, "Loi xoa du lieu benh nhan");
            sqlite3_finalize(statement);
            return false;
        }

        deletedRows += sqlite3_changes(database);
        sqlite3_finalize(statement);
        return true;
    };

    success = deleteForPatient(
        "DELETE FROM priority_checkins WHERE patient_id = ?;"
    );
    for (const auto& department : CauHinhTruyXuat::danhSachKhoa) {
        if (!success) break;
        const string deleteSql =
            "DELETE FROM retrieval." + string(department.tenBang)
            + " WHERE patient_id = ?;";
        success = deleteForPatient(deleteSql);
    }

    if (success && executeSql(database, "COMMIT;", "Khong the hoan tat giao dich xoa")) {
        if (deletedRows == 0) {
            cout << "Khong tim thay benh nhan ID " << patientId
                      << " trong hai database.\n";
        } else {
            cout << "Da xoa benh nhan ID " << patientId
                      << " khoi priority.db va truyXuat.db.\n";
        }
    } else {
        executeSql(database, "ROLLBACK;", "Khong the rollback giao dich xoa");
        success = false;
    }

    sqlite3_exec(database, "DETACH DATABASE retrieval;", nullptr, nullptr, nullptr);
    sqlite3_close(database);
    return success;
}
