#include <iostream>
#include <sqlite3.h>

int main() {
    const char* databasePath = "TRUY_XUAT_BENH_NHAN/db/truyXuat.db";
    sqlite3* database = nullptr;
    if (sqlite3_open_v2(
            databasePath,
            &database,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
            nullptr
        ) != SQLITE_OK) {
        std::cerr << "Khong mo duoc truyXuat.db: "
                  << (database ? sqlite3_errmsg(database) : "loi SQLite") << '\n';
        if (database) sqlite3_close(database);
        return 1;
    }

    const char* createTablesSql = R"(
        BEGIN TRANSACTION;
        CREATE TABLE IF NOT EXISTS queue_khoa_cap_cuu (
            retrieval_order INTEGER PRIMARY KEY,
            checkin_id INTEGER NOT NULL UNIQUE,
            patient_id INTEGER NOT NULL,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            base_priority INTEGER NOT NULL,
            current_priority INTEGER NOT NULL,
            last_update TEXT
        );
        CREATE TABLE IF NOT EXISTS queue_khoa_noi (
            retrieval_order INTEGER PRIMARY KEY,
            checkin_id INTEGER NOT NULL UNIQUE,
            patient_id INTEGER NOT NULL,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            base_priority INTEGER NOT NULL,
            current_priority INTEGER NOT NULL,
            last_update TEXT
        );
        CREATE TABLE IF NOT EXISTS queue_khoa_ngoai (
            retrieval_order INTEGER PRIMARY KEY,
            checkin_id INTEGER NOT NULL UNIQUE,
            patient_id INTEGER NOT NULL,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            base_priority INTEGER NOT NULL,
            current_priority INTEGER NOT NULL,
            last_update TEXT
        );
        CREATE TABLE IF NOT EXISTS queue_khoa_tim_mach (
            retrieval_order INTEGER PRIMARY KEY,
            checkin_id INTEGER NOT NULL UNIQUE,
            patient_id INTEGER NOT NULL,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            base_priority INTEGER NOT NULL,
            current_priority INTEGER NOT NULL,
            last_update TEXT
        );
        CREATE TABLE IF NOT EXISTS queue_khoa_nhi (
            retrieval_order INTEGER PRIMARY KEY,
            checkin_id INTEGER NOT NULL UNIQUE,
            patient_id INTEGER NOT NULL,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            base_priority INTEGER NOT NULL,
            current_priority INTEGER NOT NULL,
            last_update TEXT
        );
        CREATE TABLE IF NOT EXISTS queue_khoa_san (
            retrieval_order INTEGER PRIMARY KEY,
            checkin_id INTEGER NOT NULL UNIQUE,
            patient_id INTEGER NOT NULL,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            base_priority INTEGER NOT NULL,
            current_priority INTEGER NOT NULL,
            last_update TEXT
        );
        CREATE TABLE IF NOT EXISTS queue_khoa_tai_mui_hong (
            retrieval_order INTEGER PRIMARY KEY,
            checkin_id INTEGER NOT NULL UNIQUE,
            patient_id INTEGER NOT NULL,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            base_priority INTEGER NOT NULL,
            current_priority INTEGER NOT NULL,
            last_update TEXT
        );
        CREATE TABLE IF NOT EXISTS queue_khoa_mat (
            retrieval_order INTEGER PRIMARY KEY,
            checkin_id INTEGER NOT NULL UNIQUE,
            patient_id INTEGER NOT NULL,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            base_priority INTEGER NOT NULL,
            current_priority INTEGER NOT NULL,
            last_update TEXT
        );
        CREATE TABLE IF NOT EXISTS queue_khoa_da_lieu (
            retrieval_order INTEGER PRIMARY KEY,
            checkin_id INTEGER NOT NULL UNIQUE,
            patient_id INTEGER NOT NULL,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            base_priority INTEGER NOT NULL,
            current_priority INTEGER NOT NULL,
            last_update TEXT
        );
        CREATE TABLE IF NOT EXISTS queue_khoa_than_kinh (
            retrieval_order INTEGER PRIMARY KEY,
            checkin_id INTEGER NOT NULL UNIQUE,
            patient_id INTEGER NOT NULL,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            base_priority INTEGER NOT NULL,
            current_priority INTEGER NOT NULL,
            last_update TEXT
        );
        COMMIT;
    )";

    char* errorMessage = nullptr;
    if (sqlite3_exec(database, createTablesSql, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        std::cerr << "Loi tao bang truy xuat: "
                  << (errorMessage ? errorMessage : sqlite3_errmsg(database)) << '\n';
        sqlite3_free(errorMessage);
        sqlite3_exec(database, "ROLLBACK;", nullptr, nullptr, nullptr);
        sqlite3_close(database);
        return 1;
    }

    sqlite3_close(database);
    std::cout << "Tao bang truy xuat thanh cong.\n";
    return 0;
}
