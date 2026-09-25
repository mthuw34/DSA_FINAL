#include <iostream>
#include <sqlite3.h>

using namespace std;

int main()
{
    sqlite3* db = nullptr;

    const char* dbPath =
        "THAY_DOI_MUC_DO_UU_TIEN/priority.db";

    if (sqlite3_open(dbPath, &db) != SQLITE_OK)
    {
        cerr << "Khong mo duoc priority.db\n";
        return 1;
    }

    const char* sql = R"(

        CREATE TABLE IF NOT EXISTS priority_checkins
        (
            checkin_id INTEGER PRIMARY KEY,

            patient_id INTEGER NOT NULL,

            department TEXT NOT NULL,

            checkin_time TEXT NOT NULL,

            base_priority INTEGER NOT NULL
                CHECK(base_priority BETWEEN 1 AND 5),

            current_priority INTEGER NOT NULL
                CHECK(current_priority BETWEEN 1 AND 5),

            last_update TEXT
        );

    )";

    char* error = nullptr;

    if (sqlite3_exec(
            db,
            sql,
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

    cout << "Tao priority.db thanh cong.\n";

    sqlite3_close(db);

    return 0;
}