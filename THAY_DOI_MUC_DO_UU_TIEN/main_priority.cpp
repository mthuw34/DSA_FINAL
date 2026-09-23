#include <iostream>
#include <sqlite3.h>

#include "PriorityManager.h"

using namespace std;

int main()
{
    sqlite3* db = nullptr;

    if (sqlite3_open(
            "THAY_DOI_MUC_DO_UU_TIEN/priority.db",
            &db
        ) != SQLITE_OK)
    {
        cerr << "Khong mo duoc priority.db\n";
        return 1;
    }

    PriorityManager manager(db);

    int checkinId;
    int newPriority;

    cout << "Nhap checkin ID: ";
    cin >> checkinId;

    cout << "Nhap muc do uu tien moi (1-5): ";
    cin >> newPriority;

    manager.updatePriority(
        checkinId,
        newPriority
    );

    sqlite3_close(db);

    return 0;
}