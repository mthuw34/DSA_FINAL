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

        ALTER TABLE priority_checkins

        ADD COLUMN auto_boosted INTEGER
        NOT NULL DEFAULT 0
        CHECK(auto_boosted IN (0, 1));

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
        cout << "Cot auto_boosted co the da ton tai.\n";

        sqlite3_free(error);
    }
    else
    {
        cout << "Them cot auto_boosted thanh cong.\n";
    }

    sqlite3_close(db);

    return 0;
}