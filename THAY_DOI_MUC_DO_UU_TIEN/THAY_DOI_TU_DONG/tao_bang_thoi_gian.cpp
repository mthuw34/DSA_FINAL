#include <iostream>
#include <sqlite3.h>

using namespace std;

int main()
{
    sqlite3* db = nullptr;

    const char* dbPath =
        "THAY_DOI_MUC_DO_UU_TIEN/priority.db";

    // Chi mo database da ton tai
    if (sqlite3_open_v2(
            dbPath,
            &db,
            SQLITE_OPEN_READWRITE,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Khong mo duoc priority.db\n";

        if (db != nullptr)
            sqlite3_close(db);

        return 1;
    }

    // Bat khoa ngoai
    sqlite3_exec(
        db,
        "PRAGMA foreign_keys = ON;",
        nullptr,
        nullptr,
        nullptr
    );

    // Bang tong thoi gian cho
    const char* sqlWaiting = R"(

        CREATE TABLE IF NOT EXISTS waiting_time_progress
        (
            checkin_id INTEGER PRIMARY KEY,

            waiting_seconds INTEGER
                NOT NULL DEFAULT 0
                CHECK(waiting_seconds >= 0),

            FOREIGN KEY(checkin_id)
                REFERENCES priority_checkins(checkin_id)
                ON DELETE CASCADE
        );

    )";

    // Bang lich su cac moc da xu ly
    const char* sqlHistory = R"(

        CREATE TABLE IF NOT EXISTS auto_priority_history
        (
            checkin_id INTEGER NOT NULL,

            waiting_hour INTEGER NOT NULL
                CHECK(waiting_hour > 0),

            updated_at TEXT NOT NULL
                DEFAULT (datetime('now', 'localtime')),

            PRIMARY KEY(checkin_id, waiting_hour),

            FOREIGN KEY(checkin_id)
                REFERENCES priority_checkins(checkin_id)
                ON DELETE CASCADE
        );

    )";

    char* error = nullptr;

    // Tao bang 1
    if (sqlite3_exec(
            db,
            sqlWaiting,
            nullptr,
            nullptr,
            &error
        ) != SQLITE_OK)
    {
        cerr << "Loi tao bang thoi gian: "
             << error << '\n';

        sqlite3_free(error);
        sqlite3_close(db);

        return 1;
    }

    // Tao bang 2
    if (sqlite3_exec(
            db,
            sqlHistory,
            nullptr,
            nullptr,
            &error
        ) != SQLITE_OK)
    {
        cerr << "Loi tao bang lich su: "
             << error << '\n';

        sqlite3_free(error);
        sqlite3_close(db);

        return 1;
    }

    cout << "Tao hai bang thanh cong.\n";

    sqlite3_close(db);

    return 0;
}