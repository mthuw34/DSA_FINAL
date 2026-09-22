#include <iostream>
#include <sqlite3.h>

#include "PrioritySync.h"

using namespace std;

int main()
{
    sqlite3* hospitalDb = nullptr;
    sqlite3* priorityDb = nullptr;

    if (sqlite3_open(
            "QUAN_LY_BENH_NHAN/hospital.db",
            &hospitalDb
        ) != SQLITE_OK)
    {
        cerr << "Khong mo duoc hospital.db\n";
        return 1;
    }

    if (sqlite3_open(
            "THAY_DOI_MUC_DO_UU_TIEN/priority.db",
            &priorityDb
        ) != SQLITE_OK)
    {
        cerr << "Khong mo duoc priority.db\n";

        sqlite3_close(hospitalDb);
        return 1;
    }

    PrioritySync sync(
        hospitalDb,
        priorityDb
    );

    sync.syncAll();

    sqlite3_close(hospitalDb);
    sqlite3_close(priorityDb);

    return 0;
}