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
        cerr << "Khong mo duoc database.\n";
        return 1;
    }

    const char* sql = R"(

        INSERT INTO checkin_priority_state
        (
            checkin_id,
            current_priority,
            last_update
        )

        VALUES
        (
            1,
            1,
            datetime('now', 'localtime')
        )

        ON CONFLICT(checkin_id)

        DO UPDATE SET

            current_priority =
                excluded.current_priority,

            last_update =
                datetime('now', 'localtime');

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
        cerr << "Loi: "
             << error
             << '\n';

        sqlite3_free(error);
        sqlite3_close(db);

        return 1;
    }

    cout << "Cap nhat thu nghiem thanh cong.\n";

    sqlite3_close(db);

    return 0;
}