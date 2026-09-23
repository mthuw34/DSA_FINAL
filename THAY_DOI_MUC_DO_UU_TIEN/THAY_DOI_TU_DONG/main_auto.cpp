#include <iostream>
#include <sqlite3.h>

#include "AutoPriorityManager.h"

using namespace std;

int main()
{
    sqlite3* db = nullptr;

    const char* dbPath =
        "THAY_DOI_MUC_DO_UU_TIEN/priority.db";

    if (
        sqlite3_open(
            dbPath,
            &db
        )
        != SQLITE_OK
    )
    {
        cerr
            << "Khong mo duoc priority.db\n";

        return 1;
    }

    AutoPriorityManager manager(db);

    if (!manager.loadWaitingPatients())
    {
        sqlite3_close(db);

        return 1;
    }

    manager.showNextPatient();

    sqlite3_close(db);

    return 0;
}