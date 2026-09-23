#ifndef PRIORITY_MANAGER_H
#define PRIORITY_MANAGER_H

#include <sqlite3.h>

class PriorityManager
{
private:
    sqlite3* db;

public:
    PriorityManager(sqlite3* database);

    bool updatePriority(
        int checkinId,
        int newPriority
    );

    bool getPriority(
        int checkinId,
        int& basePriority,
        int& currentPriority
    );
};

#endif