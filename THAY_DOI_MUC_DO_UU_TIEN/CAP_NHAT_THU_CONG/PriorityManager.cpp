#include "PriorityManager.h"

#include <iostream>

using namespace std;

PriorityManager::PriorityManager(sqlite3* database)
{
    db = database;
}

bool PriorityManager::getPriority(
    int checkinId,
    int& basePriority,
    int& currentPriority
)
{
    const char* sql = R"(

        SELECT
            base_priority,
            current_priority

        FROM priority_checkins

        WHERE checkin_id = ?;

    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Loi truy van priority.\n";
        return false;
    }

    sqlite3_bind_int(
        stmt,
        1,
        checkinId
    );

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        basePriority =
            sqlite3_column_int(stmt, 0);

        currentPriority =
            sqlite3_column_int(stmt, 1);

        sqlite3_finalize(stmt);

        return true;
    }

    sqlite3_finalize(stmt);

    return false;
}

bool PriorityManager::updatePriority(
    int checkinId,
    int newPriority
)
{
    if (newPriority < 1 || newPriority > 5)
    {
        cout << "Muc do uu tien phai tu 1 den 5.\n";
        return false;
    }

    int basePriority;
    int currentPriority;

    if (!getPriority(
            checkinId,
            basePriority,
            currentPriority
        ))
    {
        cout << "Khong tim thay checkin_id = "
             << checkinId
             << '\n';

        return false;
    }

    const char* sql = R"(

        UPDATE priority_checkins

        SET
            current_priority = ?,
            last_update = datetime('now', 'localtime')

        WHERE checkin_id = ?;

    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Loi cap nhat priority.\n";
        return false;
    }

    sqlite3_bind_int(
        stmt,
        1,
        newPriority
    );

    sqlite3_bind_int(
        stmt,
        2,
        checkinId
    );

    bool success =
        sqlite3_step(stmt) == SQLITE_DONE;

    sqlite3_finalize(stmt);

    if (success)
    {
        cout << "Cap nhat thanh cong.\n";
        cout << "Check-in ID: "
             << checkinId
             << '\n';

        cout << "Priority cu: "
             << currentPriority
             << '\n';

        cout << "Priority moi: "
             << newPriority
             << '\n';
    }

    return success;
}