#include <iostream>
#include <sqlite3.h>

int main() {
    sqlite3* db;

    if (sqlite3_open("hospital.db", &db) != SQLITE_OK) {
        std::cout << "Khong mo duoc database\n";
        return 1;
    }

    int id;
    std::cout << "Nhap ID benh nhan can xoa: ";
    std::cin >> id;

    sqlite3_stmt* stmt;

    const char* sql =
        "DELETE FROM patients WHERE id = ?;";

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        std::cout << "Da xoa benh nhan co ID = "
                  << id << "\n";
    } else {
        std::cout << "Xoa that bai\n";
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}