#include "PrioritySync.h"

#include <iostream>
#include <vector>
#include <unordered_set>
#include <string>

using namespace std;

PrioritySync::PrioritySync(
    sqlite3* hospital,
    sqlite3* priority
)
{
    hospitalDb = hospital;
    priorityDb = priority;
}

bool PrioritySync::syncAll()
{
    // =============================
    // 1. DOC DU LIEU TU hospital.db
    // =============================

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

    // ===================================
    // 2. THEM / CAP NHAT VAO priority.db
    // ===================================

    const char* upsertSql = R"(

        INSERT INTO priority_checkins
        (
            checkin_id,
            patient_id,
            department,
            checkin_time,
            base_priority,
            current_priority
        )

        VALUES (?, ?, ?, ?, ?, ?)

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

    while (sqlite3_step(selectStmt) == SQLITE_ROW)
    {
        int checkinId =
            sqlite3_column_int(selectStmt, 0);

        int patientId =
            sqlite3_column_int(selectStmt, 1);

        const unsigned char* departmentText =
            sqlite3_column_text(selectStmt, 2);

        string department =
            departmentText
            ? reinterpret_cast<const char*>(departmentText)
            : "";

        const unsigned char* timeText =
            sqlite3_column_text(selectStmt, 3);

        string checkinTime =
            timeText
            ? reinterpret_cast<const char*>(timeText)
            : "";

        int basePriority =
            sqlite3_column_int(selectStmt, 4);

        validIds.insert(checkinId);

        sqlite3_reset(upsertStmt);
        sqlite3_clear_bindings(upsertStmt);

        sqlite3_bind_int(
            upsertStmt,
            1,
            checkinId
        );

        sqlite3_bind_int(
            upsertStmt,
            2,
            patientId
        );

        sqlite3_bind_text(
            upsertStmt,
            3,
            department.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        sqlite3_bind_text(
            upsertStmt,
            4,
            checkinTime.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        sqlite3_bind_int(
            upsertStmt,
            5,
            basePriority
        );

        // Neu check-in moi:
        // current_priority ban dau = base_priority
        sqlite3_bind_int(
            upsertStmt,
            6,
            basePriority
        );

        if (sqlite3_step(upsertStmt) != SQLITE_DONE)
        {
            cerr << "Loi dong bo checkin_id = "
                 << checkinId
                 << '\n';

            sqlite3_finalize(selectStmt);
            sqlite3_finalize(upsertStmt);

            return false;
        }
    }

    sqlite3_finalize(selectStmt);
    sqlite3_finalize(upsertStmt);

    // =====================================
    // 3. XOA CAC CHECK-IN KHONG CON O GOC
    // =====================================

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
        int id =
            sqlite3_column_int(readStmt, 0);

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

        sqlite3_bind_int(
            deleteStmt,
            1,
            id
        );

        sqlite3_step(deleteStmt);
    }

    sqlite3_finalize(deleteStmt);

    cout << "Dong bo thanh cong.\n";

    return true;
}
