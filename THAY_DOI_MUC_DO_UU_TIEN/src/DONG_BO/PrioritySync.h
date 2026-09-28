#ifndef PRIORITY_SYNC_H
#define PRIORITY_SYNC_H

#include <sqlite3.h>

class PrioritySync
{
private:
    sqlite3* hospitalDb;
    sqlite3* priorityDb;

public:
    PrioritySync(sqlite3* hospital, sqlite3* priority);

    bool syncAll();
};

#endif  