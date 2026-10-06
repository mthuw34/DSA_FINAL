#include <iostream>
#include <filesystem>
#include <sqlite3.h>

#include "PrioritySync.h"

using namespace std;

int main()
{
    sqlite3* hospitalDb = nullptr;
    sqlite3* priorityDb = nullptr;
    sqlite3* examsDb = nullptr;

    if (sqlite3_open(
            "QUAN_LY_BENH_NHAN/db/hospital.db",
            &hospitalDb
        ) != SQLITE_OK)
    {
        cerr << "Khong mo duoc hospital.db\n";
        return 1;
    }

    if (sqlite3_open(
            "THAY_DOI_MUC_DO_UU_TIEN/db/priority.db",
            &priorityDb
        ) != SQLITE_OK)
    {
        cerr << "Khong mo duoc priority.db\n";

        sqlite3_close(hospitalDb);
        return 1;
    }

    if (std::filesystem::exists("DANG_KHAM/db/dangKham.db") &&
        sqlite3_open_v2("DANG_KHAM/db/dangKham.db", &examsDb, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
    {
        cerr << "Khong mo duoc dangKham.db\n";
        if (examsDb) sqlite3_close(examsDb);
        sqlite3_close(hospitalDb);
        sqlite3_close(priorityDb);
        return 1;
    }

    PrioritySync sync(hospitalDb, priorityDb, examsDb);
    const bool success = sync.syncAll();

    if (examsDb) sqlite3_close(examsDb);
    sqlite3_close(hospitalDb);
    sqlite3_close(priorityDb);

    return success ? 0 : 1;
}