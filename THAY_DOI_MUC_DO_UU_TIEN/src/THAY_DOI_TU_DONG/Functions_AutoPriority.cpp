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

// Giới hạn mức ưu tiên tự động. Mức ưu tiên cao hơn sẽ được giảm xuống.
static constexpr int AUTO_PRIORITY_LIMIT = 2;

// Chuyển đổi chuỗi thời gian sang time_t. 
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

// Chuyển đổi time_t sang chuỗi thời gian.
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

// Tải dữ liệu bệnh nhân từ priority.db vào heap.
void loadPatients(sqlite3* db, AutoPriorityHeap& heap)
{
    // Đọc danh sách check-in từ priority.db.
    vector<priority_storage::Record> records;
    if (!priority_storage::loadRecords(db, records))
    {
        return;
    }

    // Tạo heap từ các bản ghi có mức ưu tiên cao hơn giới hạn và có last_update.
    for (const priority_storage::Record& record : records)
    {
        if (record.currentPriority <= AUTO_PRIORITY_LIMIT || !record.hasLastUpdate)
        {
            continue;
        }

        time_t lastUpdateTime;
        if (!parseTime(record.lastUpdate, lastUpdateTime)) continue;

        // Tính toán thời gian tiếp theo khi mức ưu tiên được tăng lên.
        AutoPriorityItem item;
        item.checkinId = record.checkinId;
        item.nextBoostTime = calculateNextBoostTime(
            lastUpdateTime, record.department == "Khoa Cap cuu");
        heap.insert(item);
    }
}

// Thực hiện cập nhật mức ưu tiên tự động dựa trên heap.
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

    // Đọc danh sách check-in từ priority.db.
    vector<priority_storage::Record> records;
    if (!priority_storage::loadRecords(db, records))
    {
        priority_storage::execute(db, "ROLLBACK;", "Loi rollback priority.db");
        return;
    }

    // Tạo bảng tra cứu vị trí của các bản ghi trong vector.
    unordered_map<int, size_t> recordIndexes;
    for (size_t i = 0; i < records.size(); ++i)
    {
        recordIndexes.emplace(records[i].checkinId, i);
    }

    // Tạo danh sách các thay đổi để thông báo sau khi cập nhật.
    struct PriorityChange
    {
        int checkinId;
        int oldPriority;
        int newPriority;
    };

    // Danh sách các thay đổi mức ưu tiên.
    vector<PriorityChange> changes;

    // Lấy thời gian hiện tại và kiểm tra các bản ghi trong heap.
    time_t now = time(nullptr);
    AutoPriorityItem item;

    // Lặp qua các bản ghi trong heap và cập nhật mức ưu tiên nếu cần.
    while (heap.peek(item) && item.nextBoostTime <= now)
    {
        heap.extractMin(item);

        auto found = recordIndexes.find(item.checkinId);
        if (found == recordIndexes.end())
        {
            continue;
        }

        // Lấy bản ghi từ vector.
        priority_storage::Record& record = records[found->second];
        if (record.currentPriority <= AUTO_PRIORITY_LIMIT || !record.hasLastUpdate)
        {
            continue;
        }

        time_t lastUpdateTime;
        if (!parseTime(record.lastUpdate, lastUpdateTime)) continue;

        const bool emergencyDepartment = record.department == "Khoa Cap cuu";
        time_t correctBoostTime = calculateNextBoostTime(lastUpdateTime, emergencyDepartment);

        // Nếu thời gian tiếp theo tính toán khác với giá trị trong heap, cập nhật lại heap.
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

        // Nếu mức ưu tiên vẫn còn cao hơn giới hạn, tính toán thời gian tiếp theo và đưa vào heap.
        if (record.currentPriority > AUTO_PRIORITY_LIMIT)
        {
            AutoPriorityItem nextItem;
            nextItem.checkinId = record.checkinId;
            nextItem.nextBoostTime = calculateNextBoostTime(
                item.nextBoostTime, emergencyDepartment);
            heap.insert(nextItem);
        }
    }

    // Nếu có thay đổi, ghi lại vào priority.db.
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
