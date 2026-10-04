#ifndef PRIORITY_MANAGER_H
#define PRIORITY_MANAGER_H

#include <sqlite3.h>

class PriorityManager
{
private:
    // Kết nối đến database ưu tiên.
    sqlite3* db;

public:
    // Khởi tạo đối tượng quản lý ưu tiên.
    PriorityManager(sqlite3* database);

    // Lấy mức ưu tiên gốc và mức hiện tại.
    bool getPriority(int checkinId, int& basePriority, int& currentPriority);

    // Cập nhật mức ưu tiên thủ công.
    bool updatePriority(int checkinId, int newPriority);
};

#endif