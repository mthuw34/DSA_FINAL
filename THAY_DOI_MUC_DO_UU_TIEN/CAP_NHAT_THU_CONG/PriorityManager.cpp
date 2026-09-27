#include "PriorityManager.h"

#include <iostream>
#include <sqlite3.h>

using namespace std;


// ==================================================
// KHOI TAO
// ==================================================

PriorityManager::PriorityManager(sqlite3* database)
{
    db = database;
}


// ==================================================
// KIEM TRA CHECK-IN ID CO TON TAI KHONG
// ==================================================

bool PriorityManager::checkinExists(int checkinId)
{
    const char* sql = R"(

        SELECT 1

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
        return false;
    }


    sqlite3_bind_int(
        stmt,
        1,
        checkinId
    );


    bool exists =
        sqlite3_step(stmt)
        == SQLITE_ROW;


    sqlite3_finalize(stmt);


    return exists;
}


// ==================================================
// LAY PRIORITY GOC VA PRIORITY HIEN TAI
// ==================================================

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
        return false;
    }


    sqlite3_bind_int(
        stmt,
        1,
        checkinId
    );


    if (
        sqlite3_step(stmt)
        != SQLITE_ROW
    )
    {
        sqlite3_finalize(stmt);

        return false;
    }


    basePriority =
        sqlite3_column_int(
            stmt,
            0
        );


    currentPriority =
        sqlite3_column_int(
            stmt,
            1
        );


    sqlite3_finalize(stmt);


    return true;
}


// ==================================================
// CAP NHAT MUC DO UU TIEN THU CONG
// ==================================================

bool PriorityManager::updatePriority(
    int checkinId,
    int newPriority
)
{
    // ------------------------------------------
    // 1. KIEM TRA ID
    // ------------------------------------------

    if (!checkinExists(checkinId))
    {
        cout
            << "Check-in ID khong ton tai.\n";

        return false;
    }


    // ------------------------------------------
    // 2. KIEM TRA PRIORITY MOI
    // ------------------------------------------

    if (
        newPriority < 1
        ||
        newPriority > 5
    )
    {
        cout
            << "Muc do uu tien phai tu 1 den 5.\n";

        return false;
    }


    // ------------------------------------------
    // 3. DOC PRIORITY HIEN TAI
    // ------------------------------------------

    int basePriority;

    int currentPriority;


    if (!getPriority(
            checkinId,
            basePriority,
            currentPriority
        ))
    {
        cout
            << "Khong doc duoc thong tin check-in.\n";

        return false;
    }


    // ------------------------------------------
    // 4. NEU KHONG THAY DOI
    // ------------------------------------------

    if (
        newPriority
        ==
        currentPriority
    )
    {
        cout
            << "Muc do uu tien moi giong muc hien tai.\n";

        return false;
    }


    // ------------------------------------------
    // 5. CAP NHAT
    // ------------------------------------------

    const char* sql = R"(

        UPDATE priority_checkins

        SET

            current_priority = ?,

            last_processed_period =
                CAST(
                    waiting_seconds / 5400
                    AS INTEGER
                ),

            last_update =
                datetime(
                    'now',
                    'localtime'
                )

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
        cout
            << "Loi tao cau lenh cap nhat: "
            << sqlite3_errmsg(db)
            << '\n';

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
        sqlite3_step(stmt)
        == SQLITE_DONE;


    sqlite3_finalize(stmt);


    if (!success)
    {
        cout
            << "Cap nhat that bai: "
            << sqlite3_errmsg(db)
            << '\n';

        return false;
    }


    // ------------------------------------------
    // 6. THONG BAO KET QUA
    // ------------------------------------------

    cout
        << "Cap nhat thanh cong.\n";

    cout
        << "Check-in ID: "
        << checkinId
        << '\n';

    cout
        << "Priority cu: "
        << currentPriority
        << '\n';

    cout
        << "Priority moi: "
        << newPriority
        << '\n';


    return true;
}