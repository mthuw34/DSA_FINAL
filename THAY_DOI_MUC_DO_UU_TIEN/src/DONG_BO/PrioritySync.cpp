#include "PrioritySync.h"

#include <iostream>
#include <vector>
#include <unordered_set>
#include <string>

using namespace std;

PrioritySync::PrioritySync(sqlite3* hospital, sqlite3* priority )
{
    hospitalDb = hospital;
    priorityDb = priority;
}

bool PrioritySync::syncAll()
{
    // Đọc dữ liệu từ hospital.db
    const char* selectSql = R"(

        SELECT
            checkin_id,
            patient_id,
            department,
            checkin_time,
            priority

        FROM checkins;

    )";

    sqlite3_stmt* selectStmt = nullptr;

    if (sqlite3_prepare_v2(
            hospitalDb,
            selectSql,
            -1,
            &selectStmt,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Loi doc bang checkins trong hospital.db\n";
        return false;
    }

    // Thêm/ cập nhật vào priority.db
    const char* upsertSql = R"(

        INSERT INTO priority_checkins
        (
            checkin_id,
            patient_id,
            department,
            checkin_time,
            base_priority,
            current_priority,
            last_update
        )

        VALUES (?, ?, ?, ?, ?, ?, ?)

        ON CONFLICT(checkin_id)

        DO UPDATE SET
            patient_id = excluded.patient_id,
            department = excluded.department,
            checkin_time = excluded.checkin_time,
            base_priority = excluded.base_priority;

    )";

    sqlite3_stmt* upsertStmt = nullptr;

    if (sqlite3_prepare_v2(
            priorityDb,
            upsertSql,
            -1,
            &upsertStmt,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Loi prepare priority.db\n";

        sqlite3_finalize(selectStmt);
        return false;
    }

    unordered_set<int> validIds;

    // Duyệt từng dòng, mỗi lần gọi tên tiến đến 1 dòng kết quả tiếp theo
    while (sqlite3_step(selectStmt) == SQLITE_ROW)
    {
        int checkinId = sqlite3_column_int(selectStmt, 0);
        int patientId = sqlite3_column_int(selectStmt, 1);

        const unsigned char* departmentText = sqlite3_column_text(selectStmt, 2);

        string department = departmentText
            ? reinterpret_cast<const char*>(departmentText): "";

        const unsigned char* timeText = sqlite3_column_text(selectStmt, 3);

        string checkinTime = timeText
            ? reinterpret_cast<const char*>(timeText): "";

        int basePriority = sqlite3_column_int(selectStmt, 4);

        validIds.insert(checkinId);

        sqlite3_reset(upsertStmt);
        sqlite3_clear_bindings(upsertStmt);
        // gán dữ liệu
        sqlite3_bind_int(upsertStmt, 1, checkinId);
        sqlite3_bind_int(upsertStmt, 2, patientId);
        sqlite3_bind_text(upsertStmt, 3, department.c_str(), -1, SQLITE_TRANSIENT );
        sqlite3_bind_text(upsertStmt, 4, checkinTime.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(upsertStmt, 5, basePriority);

        // checkin mới: current_priority ban đầu = base_priority
        sqlite3_bind_int(upsertStmt, 6, basePriority);

        sqlite3_bind_text( upsertStmt, 7, checkinTime.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(upsertStmt) != SQLITE_DONE)
        {
            cerr << "Loi dong bo checkin_id = "
                 << checkinId << ": "
                 << sqlite3_errmsg(priorityDb) << '\n';

            sqlite3_finalize(selectStmt);
            sqlite3_finalize(upsertStmt);

            return false;
        }
    }

    sqlite3_finalize(selectStmt);
    sqlite3_finalize(upsertStmt);

    // Xóa các checkin ko còn ở database gốc
    const char* readSql =
        "SELECT checkin_id FROM priority_checkins;";

    sqlite3_stmt* readStmt = nullptr;

    if (sqlite3_prepare_v2(
            priorityDb,
            readSql,
            -1,
            &readStmt,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }

    vector<int> needDelete;

    while (sqlite3_step(readStmt) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(readStmt, 0);

        if (validIds.find(id) == validIds.end())
        {
            needDelete.push_back(id);
        }
    }

    sqlite3_finalize(readStmt);

    const char* deleteSql =
        "DELETE FROM priority_checkins WHERE checkin_id = ?;";

    sqlite3_stmt* deleteStmt = nullptr;

    if (sqlite3_prepare_v2(
            priorityDb,
            deleteSql,
            -1,
            &deleteStmt,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }

    for (int id : needDelete)
    {
        sqlite3_reset(deleteStmt);
        sqlite3_clear_bindings(deleteStmt);
        sqlite3_bind_int(deleteStmt, 1, id );
        sqlite3_step(deleteStmt);
    }

    sqlite3_finalize(deleteStmt);
    return true;
}
