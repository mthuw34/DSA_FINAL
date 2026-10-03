#include <iostream>
#include <string>
#include <sqlite3.h>
#include "../patient_validation.h"

int main() {
    sqlite3* db = nullptr;
    if (sqlite3_open("QUAN_LY_BENH_NHAN/db/hospital.db", &db) != SQLITE_OK) {
        std::cerr << "Khong mo duoc database\n";
        sqlite3_close(db);
        return 1;
    }
    if (sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return 1;
    }
    std::cout << "Nhap ID benh nhan can xoa: ";
    std::string input;
    std::getline(std::cin, input);
    int id;
    try {
        id = parseNonNegativeInt(input);
        if (id == 0) throw std::invalid_argument("ID phai lon hon 0");
    } catch (const std::exception&) {
        std::cerr << "ID khong hop le\n";
        sqlite3_close(db);
        return 1;
    }
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM patients WHERE id = ?;", -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Loi SQL: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_int(stmt, 1, id);
    const int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        std::cerr << "Xoa that bai (benh nhan co the dang check-in): " << sqlite3_errmsg(db) << '\n';
    } else if (sqlite3_changes(db) == 0) {
        std::cout << "Khong tim thay benh nhan co ID = " << id << '\n';
    } else {
        std::cout << "Da xoa benh nhan co ID = " << id << '\n';
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result == SQLITE_DONE ? 0 : 1;
}
