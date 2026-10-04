#include "PrioritySync.h"

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../LUU_MUC_UU_TIEN/PriorityStorage.h"

using namespace std;

PrioritySync::PrioritySync(sqlite3* hospital, sqlite3* priority)
{
    hospitalDb = hospital;
    priorityDb = priority;
}

bool PrioritySync::syncAll()
{
    vector<priority_storage::SourceRecord> sourceRecords;
    if (!priority_storage::loadSourceRecords(hospitalDb, sourceRecords))
    {
        return false;
    }

    sqlite3_busy_timeout(priorityDb, 5000);
    if (!priority_storage::execute(
            priorityDb,
            "BEGIN IMMEDIATE;",
            "Khong the bat dau dong bo priority.db"
        ))
    {
        return false;
    }

    vector<priority_storage::Record> records;
    if (!priority_storage::loadRecords(priorityDb, records))
    {
        priority_storage::execute(
            priorityDb,
            "ROLLBACK;",
            "Loi rollback priority.db"
        );
        return false;
    }

    unordered_map<int, size_t> recordIndexes;
    for (size_t i = 0; i < records.size(); ++i)
    {
        recordIndexes.emplace(records[i].checkinId, i);
    }

    unordered_set<int> sourceIds;
    for (const priority_storage::SourceRecord& source : sourceRecords)
    {
        sourceIds.insert(source.checkinId);

        auto existing = recordIndexes.find(source.checkinId);
        if (existing != recordIndexes.end())
        {
            priority_storage::Record& record = records[existing->second];
            record.patientId = source.patientId;
            record.department = source.department;
            record.checkinTime = source.checkinTime;
            record.basePriority = source.basePriority;
            continue;
        }

        priority_storage::Record added;
        added.checkinId = source.checkinId;
        added.patientId = source.patientId;
        added.department = source.department;
        added.checkinTime = source.checkinTime;
        added.basePriority = source.basePriority;
        added.currentPriority = source.basePriority;
        added.lastUpdate = source.checkinTime;
        added.hasLastUpdate = true;
        recordIndexes.emplace(added.checkinId, records.size());
        records.push_back(added);
    }

    records.erase(
        remove_if(
            records.begin(),
            records.end(),
            [&sourceIds](const priority_storage::Record& record)
            {
                return sourceIds.find(record.checkinId) == sourceIds.end();
            }
        ),
        records.end()
    );

    if (!priority_storage::replaceRecords(priorityDb, records))
    {
        priority_storage::execute(
            priorityDb,
            "ROLLBACK;",
            "Loi rollback priority.db"
        );
        return false;
    }

    if (!priority_storage::execute(
            priorityDb,
            "COMMIT;",
            "Loi hoan tat dong bo priority.db"
        ))
    {
        priority_storage::execute(
            priorityDb,
            "ROLLBACK;",
            "Loi rollback priority.db"
        );
        return false;
    }

    return true;
}
