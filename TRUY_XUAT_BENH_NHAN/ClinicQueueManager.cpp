#include "ClinicQueueManager.h"
#include <sqlite3.h>

ClinicQueueManager::ClinicQueueManager() {
    loadPatientsFromDatabase();
}

bool ClinicQueueManager::exportRetrievalDatabase(sqlite3* sourceDatabase) {
    const char* outputPath = "TRUY_XUAT_BENH_NHAN/truyXuat.db";
    sqlite3* outputDatabase = nullptr;

    if (sqlite3_open_v2(
            outputPath,
            &outputDatabase,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
            nullptr
        ) != SQLITE_OK) {
        if (outputDatabase != nullptr) {
            sqlite3_close(outputDatabase);
        }
        return false;
    }

    const char* createOutputSchemaSql = R"(
        PRAGMA foreign_keys = ON;
        DROP TABLE IF EXISTS retrieval_queue;
        DROP TABLE IF EXISTS departments;

        CREATE TABLE departments (
            department_order INTEGER PRIMARY KEY,
            department TEXT NOT NULL UNIQUE
        );

        INSERT INTO departments (department_order, department) VALUES
            (1, 'Khoa Cap cuu'), (2, 'Khoa Noi'),
            (3, 'Khoa Ngoai'), (4, 'Khoa Tim mach'),
            (5, 'Khoa Nhi'), (6, 'Khoa San'),
            (7, 'Khoa Tai Mui Hong'), (8, 'Khoa Mat'),
            (9, 'Khoa Da lieu'), (10, 'Khoa Than kinh');

        CREATE TABLE retrieval_queue (
            checkin_id INTEGER PRIMARY KEY,
            patient_id INTEGER NOT NULL,
            patient_name TEXT NOT NULL,
            birth_date TEXT,
            age INTEGER,
            gender TEXT,
            hometown TEXT,
            address TEXT,
            phone TEXT,
            department_order INTEGER NOT NULL,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            priority INTEGER NOT NULL,
            FOREIGN KEY (department_order) REFERENCES departments(department_order)
        );
    )";

    char* errorMessage = nullptr;
    if (sqlite3_exec(outputDatabase, createOutputSchemaSql, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        sqlite3_free(errorMessage);
        sqlite3_close(outputDatabase);
        return false;
    }

    const char* query = R"(
        SELECT c.checkin_id, c.patient_id, p.name, p.birth_date, p.age,
               p.gender, p.hometown, p.address, p.phone,
               CASE c.department
                   WHEN 'Khoa Cap cuu' THEN 1 WHEN 'Khoa Noi' THEN 2
                   WHEN 'Khoa Ngoai' THEN 3 WHEN 'Khoa Tim mach' THEN 4
                   WHEN 'Khoa Nhi' THEN 5 WHEN 'Khoa San' THEN 6
                   WHEN 'Khoa Tai Mui Hong' THEN 7 WHEN 'Khoa Mat' THEN 8
                   WHEN 'Khoa Da lieu' THEN 9 WHEN 'Khoa Than kinh' THEN 10
               END,
               c.department, c.checkin_time, c.priority
        FROM checkins AS c
        JOIN patients AS p ON p.id = c.patient_id
        WHERE c.department IN (
            'Khoa Cap cuu', 'Khoa Noi', 'Khoa Ngoai', 'Khoa Tim mach',
            'Khoa Nhi', 'Khoa San', 'Khoa Tai Mui Hong', 'Khoa Mat',
            'Khoa Da lieu', 'Khoa Than kinh'
        )
        ORDER BY
            CASE c.department
                WHEN 'Khoa Cap cuu' THEN 1 WHEN 'Khoa Noi' THEN 2
                WHEN 'Khoa Ngoai' THEN 3 WHEN 'Khoa Tim mach' THEN 4
                WHEN 'Khoa Nhi' THEN 5 WHEN 'Khoa San' THEN 6
                WHEN 'Khoa Tai Mui Hong' THEN 7 WHEN 'Khoa Mat' THEN 8
                WHEN 'Khoa Da lieu' THEN 9 WHEN 'Khoa Than kinh' THEN 10
            END,
            c.priority ASC, c.checkin_time ASC, c.checkin_id ASC;
    )";

    sqlite3_stmt* sourceStatement = nullptr;
    sqlite3_stmt* outputStatement = nullptr;
    const bool statementsPrepared =
        sqlite3_prepare_v2(sourceDatabase, query, -1, &sourceStatement, nullptr) == SQLITE_OK &&
        sqlite3_prepare_v2(
            outputDatabase,
            "INSERT INTO retrieval_queue VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
            -1,
            &outputStatement,
            nullptr
        ) == SQLITE_OK;

    if (!statementsPrepared) {
        sqlite3_finalize(sourceStatement);
        sqlite3_finalize(outputStatement);
        sqlite3_close(outputDatabase);
        return false;
    }

    bool success = true;
    int sourceResult = SQLITE_ROW;
    while ((sourceResult = sqlite3_step(sourceStatement)) == SQLITE_ROW) {
        sqlite3_reset(outputStatement);
        sqlite3_clear_bindings(outputStatement);

        for (int column = 0; column < 13; ++column) {
            if (column == 0 || column == 1 || column == 4 || column == 9 || column == 12) {
                sqlite3_bind_int(outputStatement, column + 1, sqlite3_column_int(sourceStatement, column));
            } else if (sqlite3_column_type(sourceStatement, column) == SQLITE_NULL) {
                sqlite3_bind_null(outputStatement, column + 1);
            } else {
                sqlite3_bind_text(
                    outputStatement,
                    column + 1,
                    reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, column)),
                    -1,
                    SQLITE_TRANSIENT
                );
            }
        }

        if (sqlite3_step(outputStatement) != SQLITE_DONE) {
            success = false;
            break;
        }
    }

    if (sourceResult != SQLITE_DONE) {
        success = false;
    }

    sqlite3_finalize(sourceStatement);
    sqlite3_finalize(outputStatement);
    sqlite3_close(outputDatabase);
    return success;
}

void ClinicQueueManager::loadPatientsFromDatabase() {
    const char* databasePath = "QUAN_LY_BENH_NHAN/hospital.db";
    sqlite3* database = nullptr;

    if (sqlite3_open_v2(databasePath, &database, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        if (database != nullptr) {
            sqlite3_close(database);
        }
        return;
    }

    exportRetrievalDatabase(database);
    sqlite3_close(database);
}
