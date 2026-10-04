#include "PriorityManager.h"
#include <iostream>
#include <vector>
#include "../LUU_MUC_UU_TIEN/PriorityStorage.h"

using namespace std;

// Tìm kiếm tuyến tính theo check-in ID.
static int findCheckinIndex(
    const vector<priority_storage::Record>& records,
    int checkinId
)
{
    for (int i = 0; i < static_cast<int>(records.size()); i++)
    {
        if (records[i].checkinId == checkinId)
        {
            return i;
        }
    }

    return -1;
}

// Hủy các thay đổi trong giao dịch hiện tại.
static void rollbackUpdate(sqlite3* db)
{
    priority_storage::execute(
        db,
        "ROLLBACK;",
        "Loi rollback priority.db"
    );
}

// Khởi tạo đối tượng quản lý ưu tiên.
PriorityManager::PriorityManager(sqlite3* database)
{
    db = database;
}

// Đọc mức ưu tiên gốc và mức ưu tiên hiện tại.
bool PriorityManager::getPriority(
    int checkinId,
    int& basePriority,
    int& currentPriority
)
{
    vector<priority_storage::Record> records;

    if (!priority_storage::loadRecords(db, records))
    {
        return false;
    }

    int index = findCheckinIndex(records, checkinId);

    if (index == -1)
    {
        return false;
    }

    basePriority = records[index].basePriority;
    currentPriority = records[index].currentPriority;

    return true;
}

// Cập nhật mức ưu tiên thủ công.
bool PriorityManager::updatePriority(
    int checkinId,
    int newPriority
)
{
    // Chờ tối đa khoảng 5 giây nếu database bị khóa.
    sqlite3_busy_timeout(db, 5000);

    // Bắt đầu giao dịch trước khi đọc và sửa dữ liệu.
    if (!priority_storage::execute(
            db,
            "BEGIN IMMEDIATE;",
            "Khong the bat dau cap nhat uu tien"
        ))
    {
        return false;
    }

    // Đọc danh sách check-in vào mảng động.
    vector<priority_storage::Record> records;

    if (!priority_storage::loadRecords(db, records))
    {
        rollbackUpdate(db);
        cout << "Khong doc duoc thong tin check-in.\n";
        return false;
    }

    // Tìm vị trí check-in cần cập nhật.
    int index = findCheckinIndex(records, checkinId);

    if (index == -1)
    {
        rollbackUpdate(db);
        cout << "Check-in ID khong ton tai.\n";
        return false;
    }

    // Kiểm tra mức ưu tiên mới.
    if (newPriority < 1 || newPriority > 5)
    {
        rollbackUpdate(db);
        cout << "Muc do uu tien phai tu 1 den 5.\n";
        return false;
    }

    int oldPriority = records[index].currentPriority;

    if (newPriority == oldPriority)
    {
        rollbackUpdate(db);
        cout << "Muc do uu tien moi giong muc hien tai.\n";
        return false;
    }

    // Sửa mức ưu tiên của phần tử trong mảng.
    records[index].currentPriority = newPriority;

    // Ghi danh sách đã sửa vào database.
    if (!priority_storage::replaceRecords(db, records))
    {
        rollbackUpdate(db);
        return false;
    }

    // Xác nhận lưu các thay đổi.
    if (!priority_storage::execute(
            db,
            "COMMIT;",
            "Loi hoan tat cap nhat uu tien"
        ))
    {
        rollbackUpdate(db);
        return false;
    }

    cout << "Cap nhat thanh cong.\n";
    cout << "Check-in ID: " << checkinId << '\n';
    cout << "Priority cu: " << oldPriority << '\n';
    cout << "Priority moi: " << newPriority << '\n';

    return true;
}