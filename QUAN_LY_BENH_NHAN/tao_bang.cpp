#include <iostream>
#include <sqlite3.h>

int main()
{
    sqlite3* db = nullptr;

    // Mo database
    if (sqlite3_open("hospital.db", &db) != SQLITE_OK)
    {
        std::cerr << "Khong mo duoc database: "
                  << sqlite3_errmsg(db) << '\n';

        sqlite3_close(db);
        return 1;
    }

    // =========================================
    // TAO BANG PATIENTS
    // =========================================

    const char* sqlPatients = R"(

        CREATE TABLE IF NOT EXISTS patients (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            birth_date TEXT NOT NULL,
            age INTEGER NOT NULL,
            gender TEXT,
            hometown TEXT,
            address TEXT,
            phone TEXT,
            height REAL,
            weight REAL,
            bmi REAL
        );

    )";

    char* error = nullptr;

    int result = sqlite3_exec(
        db,
        sqlPatients,
        nullptr,
        nullptr,
        &error
    );

    if (result != SQLITE_OK)
    {
        std::cerr << "Loi tao bang patients: "
                  << error << '\n';

        sqlite3_free(error);
        sqlite3_close(db);

        return 1;
    }

    std::cout << "Tao bang patients thanh cong\n";

    // =========================================
    // TAO BANG CHECKINS
    // =========================================

    const char* sqlCheckin = R"(

        CREATE TABLE IF NOT EXISTS checkins (
            checkin_id INTEGER PRIMARY KEY AUTOINCREMENT,

            patient_id INTEGER NOT NULL,

            department TEXT NOT NULL,

            checkin_time TEXT NOT NULL
                DEFAULT (datetime('now', 'localtime')),

            priority INTEGER NOT NULL
                CHECK(priority BETWEEN 1 AND 5),

            FOREIGN KEY(patient_id)
                REFERENCES patients(id)
        );

        CREATE UNIQUE INDEX IF NOT EXISTS ux_checkins_patient_id
            ON checkins(patient_id);

    )";

    error = nullptr;

    result = sqlite3_exec(
        db,
        sqlCheckin,
        nullptr,
        nullptr,
        &error
    );

    if (result != SQLITE_OK)
    {
        std::cerr << "Loi tao bang checkins: "
                  << error << '\n';

        sqlite3_free(error);
        sqlite3_close(db);

        return 1;
    }

    std::cout << "Tao bang checkins thanh cong\n";

    sqlite3_close(db);

    return 0;
}
