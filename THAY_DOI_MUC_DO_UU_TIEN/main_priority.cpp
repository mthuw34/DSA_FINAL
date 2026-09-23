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
    int basePriority;
    int currentPriority;

    // =========================
    // 1. NHAP CHECKIN ID
    // =========================

    cout << "Nhap checkin ID: ";
    cin >> checkinId;

    // =========================
    // 2. KIEM TRA ID CO TON TAI
    // =========================

    if (!manager.getPriority(
            checkinId,
            basePriority,
            currentPriority
        ))
    {
        cout << "Checkin ID khong ton tai.\n";

        sqlite3_close(db);

        return 0;
    }

    // =========================
    // 3. NEU ID HOP LE
    // =========================

    cout << "\nTim thay check-in.\n";

    cout << "Checkin ID: "
         << checkinId
         << '\n';

    cout << "Priority ban dau: "
         << basePriority
         << '\n';

    cout << "Priority hien tai: "
         << currentPriority
         << '\n';

    // =========================
    // 4. NHAP PRIORITY MOI
    // =========================

    int newPriority;

    cout << "\nNhap muc do uu tien moi (1-5): ";
    cin >> newPriority;

    // =========================
    // 5. KIEM TRA PRIORITY
    // =========================

    if (newPriority < 1 || newPriority > 5)
    {
        cout << "Muc do uu tien khong hop le.\n";

        sqlite3_close(db);

        return 0;
    }

    // =========================
    // 6. CAP NHAT
    // =========================

    manager.updatePriority(
        checkinId,
        newPriority
    );

    sqlite3_close(db);

    return 0;
}