#include <iostream>
#include <sqlite3.h>

using namespace std;

int main()
{
    sqlite3* db = nullptr;

    const char* dbPath =
        "THAY_DOI_MUC_DO_UU_TIEN/hospital_backup.db";

    if (sqlite3_open(dbPath, &db) != SQLITE_OK)
    {
        cerr << "Khong mo duoc database: "
             << sqlite3_errmsg(db)
             << '\n';

        sqlite3_close(db);
        return 1;
    }

    cout << "Mo database thanh cong.\n";

    sqlite3_exec(
        db,
        "PRAGMA foreign_keys = ON;",
        nullptr,
        nullptr,
        nullptr
    );

    const char* createTableSql = R"(

        CREATE TABLE IF NOT EXISTS checkin_priority_state
        (
            checkin_id INTEGER PRIMARY KEY,

            current_priority INTEGER NOT NULL
                CHECK(current_priority BETWEEN 1 AND 5),

            last_update TEXT NOT NULL
                DEFAULT (datetime('now', 'localtime')),

            FOREIGN KEY(checkin_id)
                REFERENCES checkins(checkin_id)
                ON DELETE CASCADE
        );

    )";

    char* error = nullptr;

    if (sqlite3_exec(
            db,
            createTableSql,
            nullptr,
            nullptr,
            &error
        ) != SQLITE_OK)
    {
        cerr << "Loi tao bang: "
             << error
             << '\n';

        sqlite3_free(error);
        sqlite3_close(db);

        return 1;
    }

    cout << "Tao bang checkin_priority_state thanh cong.\n";

    const char* createViewSql = R"(

    CREATE VIEW IF NOT EXISTS checkin_priority_view AS

    SELECT
        c.checkin_id,
        c.patient_id,
        c.department,
        c.checkin_time,

        c.priority AS base_priority,

        COALESCE(
            p.current_priority,
            c.priority
        ) AS current_priority,

        p.last_update

    FROM checkins c

    LEFT JOIN checkin_priority_state p
        ON c.checkin_id = p.checkin_id;

)";

error = nullptr;

if (sqlite3_exec(
        db,
        createViewSql,
        nullptr,
        nullptr,
        &error
    ) != SQLITE_OK)
{
    cerr << "Loi tao view: "
         << error
         << '\n';

    sqlite3_free(error);
    sqlite3_close(db);

    return 1;
}

cout << "Tao view checkin_priority_view thanh cong.\n";

    sqlite3_close(db);

    return 0;
}