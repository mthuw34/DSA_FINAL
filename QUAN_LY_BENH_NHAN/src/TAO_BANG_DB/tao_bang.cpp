#include <iostream>
#include <string>
#include <sqlite3.h>

static bool executeSql(sqlite3* db, const std::string& sql, const char* message)
{
    char* error = nullptr;
    const int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error);

    if (result == SQLITE_OK)
        return true;

    std::cerr << message << ": "
              << (error ? error : sqlite3_errmsg(db))
              << '\n';

    sqlite3_free(error);
    return false;
}

static bool columnExists(sqlite3* db, const char* table, const char* column)
{
    const std::string sql = "PRAGMA table_info(" + std::string(table) + ");";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    bool found = false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char* name = sqlite3_column_text(stmt, 1);

        if (name != nullptr &&
            std::string(reinterpret_cast<const char*>(name)) == column)
        {
            found = true;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return found;
}

static bool addColumnIfMissing(
    sqlite3* db,
    const char* table,
    const char* column,
    const char* definition
)
{
    if (columnExists(db, table, column))
        return true;

    const std::string sql =
        "ALTER TABLE " + std::string(table) +
        " ADD COLUMN " + column + " " + definition + ";";

    return executeSql(db, sql, "Loi bo sung cot");
}

int main()
{
    sqlite3* db = nullptr;

    if (sqlite3_open("QUAN_LY_BENH_NHAN/db/hospital.db", &db) != SQLITE_OK)
    {
        std::cerr << "Khong mo duoc database: "
                  << (db ? sqlite3_errmsg(db) : "loi SQLite")
                  << '\n';

        if (db != nullptr)
            sqlite3_close(db);

        return 1;
    }

    sqlite3_busy_timeout(db, 5000);

    if (!executeSql(db, "PRAGMA foreign_keys = ON;", "Khong bat duoc foreign key"))
    {
        sqlite3_close(db);
        return 1;
    }

    if (!executeSql(db, "BEGIN IMMEDIATE;", "Khong bat dau duoc giao dich"))
    {
        sqlite3_close(db);
        return 1;
    }

    bool success = executeSql(
        db,
        R"(
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
        )",
        "Loi tao bang patients"
    );

    // Ho tro database cu: CREATE TABLE IF NOT EXISTS khong tu them cot moi.
    if (success) success = addColumnIfMissing(db, "patients", "height", "REAL");
    if (success) success = addColumnIfMissing(db, "patients", "weight", "REAL");
    if (success) success = addColumnIfMissing(db, "patients", "bmi", "REAL");

    if (success)
    {
        success = executeSql(
            db,
            R"(
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
                        ON DELETE RESTRICT
                );
            )",
            "Loi tao bang checkins"
        );
    }

    if (success)
    {
        success = executeSql(
            db,
            "CREATE UNIQUE INDEX IF NOT EXISTS ux_checkins_patient_id "
            "ON checkins(patient_id);",
            "Loi tao unique index checkins"
        );
    }

    if (success)
    {
        success = executeSql(db, "COMMIT;", "Khong commit duoc giao dich");
    }

    if (!success)
    {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sqlite3_close(db);
        return 1;
    }

    std::cout << "Tao/kiem tra bang patients thanh cong\n";
    std::cout << "Tao/kiem tra bang checkins thanh cong\n";

    sqlite3_close(db);
    return 0;
}
