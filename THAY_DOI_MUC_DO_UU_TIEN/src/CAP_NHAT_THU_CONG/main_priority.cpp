#include <iostream>
#include <sqlite3.h>

#include "PriorityManager.h"

using namespace std;

int main()
{
    sqlite3* db = nullptr;

    if (sqlite3_open(
            "THAY_DOI_MUC_DO_UU_TIEN/db/priority.db",
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

    // NHẬP CHECKIN ID
    cout << "Nhap checkin ID: ";
    cin >> checkinId;

    // KIỂM TRA ID CÓ TỒN TẠI HAY KHÔNG
    if (!manager.getPriority(checkinId, basePriority, currentPriority))
    {
        cout << "Checkin ID khong ton tai.\n";
        sqlite3_close(db);
        return 0;
    }

    // NẾU ID HỢP LỆ
    cout << "\nTim thay check-in.\n";

    cout << "Checkin ID: " << checkinId << '\n';
    cout << "Priority ban dau: " << basePriority << '\n';
    cout << "Priority hien tai: " << currentPriority << '\n';

    // NHẬP MỨC ĐỘ ƯU TIÊN MỚI
    int newPriority;

    cout << "\nNhap muc do uu tien moi (1-5): ";
    cin >> newPriority;

    // KIỂM TRA MỨC ĐỘ ƯU TIÊN
    if (newPriority < 1 || newPriority > 5)
    {
        cout << "Muc do uu tien khong hop le.\n";
        sqlite3_close(db);

        return 0;
    }

    // CẬP NHẬT
    manager.updatePriority(checkinId, newPriority);
    sqlite3_close(db);

    return 0;
}