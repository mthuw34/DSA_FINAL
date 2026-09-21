#include <iostream>
#include <sqlite3.h>

int main()
{
    sqlite3* db = nullptr;

    if (sqlite3_open("hospital.db", &db) != SQLITE_OK)
    {
        std::cerr << sqlite3_errmsg(db) << '\n';
        return 1;
    }

    const char* sql = R"(

        CREATE TABLE IF NOT EXISTS patients (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            brith_date TEXT NOT NULL,
            age INTEGER NOT NULL,
            gender TEXT,
            hometown TEXT,
            address TEXT,
            phone TEXT
        );

    )";

    char* error = nullptr;

    int result = sqlite3_exec(
        db,
        sql,
        nullptr,
        nullptr,
        &error
    );

    if (result != SQLITE_OK)
    {
        std::cerr << "SQL Error: " << error << '\n';

        sqlite3_free(error);
    }
    else
    {
        std::cout << "Tao bang thanh cong\n";
    }

    sqlite3_close(db);

    return 0;
}