#pragma once
#include <sqlite3.h>

class ClinicQueueManager {
private:
    bool exportRetrievalDatabase(sqlite3* sourceDatabase);
    void loadPatientsFromDatabase();

public:
    ClinicQueueManager();
};