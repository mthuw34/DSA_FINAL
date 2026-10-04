#include "XoaBenhNhan.h"

#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <string>

using namespace std;

bool XoaBenhNhan::xoaBenhNhan(int patientId) {
    if (patientId <= 0) {
        cerr << "ID benh nhan phai lon hon 0.\n";
        return false;
    }

    const char* priorityPath = "THAY_DOI_MUC_DO_UU_TIEN/db/priority.db";
    const char* retrievalPath = "TRUY_XUAT_BENH_NHAN/db/truyXuat.db";

    ifstream priorityFile(priorityPath, ios::binary);
    ifstream retrievalFile(retrievalPath, ios::binary);
    if (!priorityFile || !retrievalFile) {
        cerr << "Khong tim thay priority.db hoac truyXuat.db. "
                "Hay chay chuong trinh tu thu muc goc workspace.\n";
        return false;
    }

    sqlite3* database = nullptr;
    if (sqlite3_open_v2(priorityPath, &database, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        cerr << "Khong mo duoc priority.db: "
             << (database ? sqlite3_errmsg(database) : "loi SQLite") << '\n';
        if (database) sqlite3_close(database);
        return false;
    }

    sqlite3_stmt* attachStatement = nullptr;
    if (sqlite3_prepare_v2(database, "ATTACH DATABASE ? AS retrieval;", -1, &attachStatement, nullptr) != SQLITE_OK) {
        cerr << "Khong the ket noi truyXuat.db: " << sqlite3_errmsg(database) << '\n';
        sqlite3_close(database);
        return false;
    }
    sqlite3_bind_text(attachStatement, 1, retrievalPath, -1, SQLITE_TRANSIENT);
    const int attachResult = sqlite3_step(attachStatement);
    sqlite3_finalize(attachStatement);
    if (attachResult != SQLITE_DONE) {
        cerr << "Khong the ket noi truyXuat.db: " << sqlite3_errmsg(database) << '\n';
        sqlite3_close(database);
        return false;
    }

    char* errorMessage = nullptr;
    if (sqlite3_exec(database, "BEGIN IMMEDIATE;", nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        cerr << "Khong the bat dau giao dich xoa: "
             << (errorMessage ? errorMessage : sqlite3_errmsg(database)) << '\n';
        sqlite3_free(errorMessage);
        sqlite3_close(database);
        return false;
    }

    int deletedRows = 0;
    bool success = true;
    const auto deleteForPatient = [&](const string& sql) {
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(database, sql.c_str(), -1, &statement, nullptr) != SQLITE_OK) {
            cerr << "Loi chuan bi cau lenh xoa: " << sqlite3_errmsg(database) << '\n';
            return false;
        }

        sqlite3_bind_int(statement, 1, patientId);
        const int result = sqlite3_step(statement);
        if (result != SQLITE_DONE) {
            cerr << "Loi xoa du lieu: " << sqlite3_errmsg(database) << '\n';
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

    static const char* retrievalTables[] = {
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

    for (const char* tableName : retrievalTables) {
        if (!success) break;
        success = deleteForPatient(
            string("DELETE FROM retrieval.") + tableName + " WHERE patient_id = ?;"
        );
    }

    if (success && sqlite3_exec(database, "COMMIT;", nullptr, nullptr, &errorMessage) == SQLITE_OK) {
        if (deletedRows == 0) {
            cout << "Khong tim thay benh nhan ID " << patientId
                 << " trong hai database.\n";
        } else {
            cout << "Da xoa benh nhan ID " << patientId
                 << " khoi priority.db va truyXuat.db.\n";
        }
    } else {
        if (success) {
            cerr << "Khong the commit giao dich xoa: "
                 << (errorMessage ? errorMessage : sqlite3_errmsg(database)) << '\n';
            sqlite3_free(errorMessage);
        }
        sqlite3_exec(database, "ROLLBACK;", nullptr, nullptr, nullptr);
        success = false;
    }

    sqlite3_exec(database, "DETACH DATABASE retrieval;", nullptr, nullptr, nullptr);
    sqlite3_close(database);
    return success;
}
