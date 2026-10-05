#ifndef PRIORITY_SYNC_H
#define PRIORITY_SYNC_H

#include <sqlite3.h>
#include <vector>

class PrioritySync
{
private:
    sqlite3* hospitalDb;
    sqlite3* priorityDb;
    sqlite3* examsDb;

public:
    //Khởi tạo đồng bộ
    PrioritySync(sqlite3* hospital, sqlite3* priority, sqlite3* exams = nullptr);
    bool syncAll(const std::vector<int>& assignedCheckins = {});
};

#endif  