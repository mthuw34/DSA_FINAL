#ifndef AUTO_PRIORITY_MANAGER_H
#define AUTO_PRIORITY_MANAGER_H

#include <sqlite3.h>

#include "AutoPriorityHeap.h"

class AutoPriorityManager
{
private:
    sqlite3* db;

    AutoPriorityHeap heap;

public:
    AutoPriorityManager(sqlite3* database);

    bool loadWaitingPatients();

    void showNextPatient();
};

#endif