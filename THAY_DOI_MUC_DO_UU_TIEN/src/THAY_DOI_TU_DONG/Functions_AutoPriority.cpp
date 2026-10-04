#include "AutoPriority.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../LUU_MUC_UU_TIEN/PriorityStorage.h"
#include "WorkingTime.h"

using namespace std;

static bool parseTime(const string& text, time_t& result)
{
    tm t = {};
    stringstream ss(text);

    ss >> get_time(&t, "%Y-%m-%d %H:%M:%S");
    if (ss.fail())
    {
        return false;
    }

    t.tm_isdst = -1;
    result = mktime(&t);
    return result != static_cast<time_t>(-1);
}

static string formatTime(time_t timestamp)
{
    tm* info = localtime(&timestamp);
    if (info == nullptr) return "";
    char text[20];
    if (strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", info) == 0)
    {
        return "";
    }

    return text;
}

void loadPatients(sqlite3* db, AutoPriorityHeap& heap)
{
    vector<priority_storage::Record> records;
    if (!priority_storage::loadRecords(db, records))
    {
        return;
    }

    for (const priority_storage::Record& record : records)
    {
        if (record.currentPriority <= 1 || !record.hasLastUpdate)
        {
            continue;
        }

        time_t lastUpdateTime;
        if (!parseTime(record.lastUpdate, lastUpdateTime)) continue;

        AutoPriorityItem item;
        item.checkinId = record.checkinId;
        item.nextBoostTime = calculateNextBoostTime(lastUpdateTime);
        heap.insert(item);
    }
}

void processAuto(sqlite3* db, AutoPriorityHeap& heap)
{
    sqlite3_busy_timeout(db, 5000);
    if (!priority_storage::execute(
            db,
            "BEGIN IMMEDIATE;",
            "Khong the bat dau cap nhat uu tien tu dong"
        ))
    {
        return;
    }

    vector<priority_storage::Record> records;
    if (!priority_storage::loadRecords(db, records))
    {
        priority_storage::execute(db, "ROLLBACK;", "Loi rollback priority.db");
        return;
    }

    unordered_map<int, size_t> recordIndexes;
    for (size_t i = 0; i < records.size(); ++i)
    {
        recordIndexes.emplace(records[i].checkinId, i);
    }

    struct PriorityChange
    {
        int checkinId;
        int oldPriority;
        int newPriority;
    };
    vector<PriorityChange> changes;

    time_t now = time(nullptr);
    AutoPriorityItem item;

    while (heap.peek(item) && item.nextBoostTime <= now)
    {
        heap.extractMin(item);

        auto found = recordIndexes.find(item.checkinId);
        if (found == recordIndexes.end())
        {
            continue;
        }

        priority_storage::Record& record = records[found->second];
        if (record.currentPriority <= 1 || !record.hasLastUpdate)
        {
            continue;
        }

        time_t lastUpdateTime;
        if (!parseTime(record.lastUpdate, lastUpdateTime)) continue;

        time_t correctBoostTime = calculateNextBoostTime(lastUpdateTime);
        if (item.nextBoostTime != correctBoostTime)
        {
            item.nextBoostTime = correctBoostTime;
            heap.insert(item);
            continue;
        }

        int oldPriority = record.currentPriority;
        record.currentPriority -= 1;
        record.lastUpdate = formatTime(item.nextBoostTime);
        record.hasLastUpdate = true;
        changes.push_back({record.checkinId, oldPriority, record.currentPriority});

        if (record.currentPriority > 1)
        {
            AutoPriorityItem nextItem;
            nextItem.checkinId = record.checkinId;
            nextItem.nextBoostTime = calculateNextBoostTime(item.nextBoostTime);
            heap.insert(nextItem);
        }
    }

    if (!changes.empty() && !priority_storage::replaceRecords(db, records))
    {
        priority_storage::execute(db, "ROLLBACK;", "Loi rollback priority.db");
        return;
    }

    if (!priority_storage::execute(
            db,
            "COMMIT;",
            "Loi hoan tat cap nhat uu tien tu dong"
        ))
    {
        priority_storage::execute(db, "ROLLBACK;", "Loi rollback priority.db");
        return;
    }

    for (const PriorityChange& change : changes)
    {
        cout << "Checkin ID " << change.checkinId << ": "
             << change.oldPriority << " -> " << change.newPriority << '\n';
    }
}
