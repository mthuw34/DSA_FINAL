#include "ClinicQueueManager.h"
#include <cstdio>
#include <sqlite3.h>
#include <string>
#include <vector>

using namespace std;

namespace {
struct RetrievalRecord {
    int checkinId;
    int patientId;
    string patientName;
    string birthDate;
    int age;
    string gender;
    string hometown;
    string address;
    string phone;
    int departmentOrder;
    string department;
    string checkinTime;
    int priority;
};

int departmentOrder(const string& department) {
    static const vector<string> departments = {
        "Khoa Cap cuu", "Khoa Noi", "Khoa Ngoai", "Khoa Tim mach",
        "Khoa Nhi", "Khoa San", "Khoa Tai Mui Hong", "Khoa Mat",
        "Khoa Da lieu", "Khoa Than kinh"
    };

    for (int index = 0; index < static_cast<int>(departments.size()); ++index) {
        if (departments[index] == department) {
            return index + 1;
        }
    }

    return 99;
}

bool comesBefore(const RetrievalRecord& left, const RetrievalRecord& right) {
    if (left.departmentOrder != right.departmentOrder) {
        return left.departmentOrder < right.departmentOrder;
    }
    if (left.priority != right.priority) {
        return left.priority < right.priority;
    }
    if (left.checkinTime != right.checkinTime) {
        return left.checkinTime < right.checkinTime;
    }
    return left.checkinId < right.checkinId;
}

void merge(
    vector<RetrievalRecord>& records,
    vector<RetrievalRecord>& buffer,
    int left,
    int middle,
    int right
) {
    int first = left;
    int second = middle + 1;
    int target = left;

    while (first <= middle && second <= right) {
        if (comesBefore(records[first], records[second])) {
            buffer[target++] = records[first++];
        } else {
            buffer[target++] = records[second++];
        }
    }

    while (first <= middle) {
        buffer[target++] = records[first++];
    }
    while (second <= right) {
        buffer[target++] = records[second++];
    }

    for (int index = left; index <= right; ++index) {
        records[index] = buffer[index];
    }
}

void mergeSort(
    vector<RetrievalRecord>& records,
    vector<RetrievalRecord>& buffer,
    int left,
    int right
) {
    if (left >= right) {
        return;
    }

    const int middle = left + (right - left) / 2;
    mergeSort(records, buffer, left, middle);
    mergeSort(records, buffer, middle + 1, right);
    merge(records, buffer, left, middle, right);
}
}

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

    const char* tableNames[] = {
        "queue_khoa_cap_cuu", "queue_khoa_noi", "queue_khoa_ngoai",
        "queue_khoa_tim_mach", "queue_khoa_nhi", "queue_khoa_san",
        "queue_khoa_tai_mui_hong", "queue_khoa_mat", "queue_khoa_da_lieu",
        "queue_khoa_than_kinh"
    };

    const char* dropTablesSql = R"(
        PRAGMA foreign_keys = ON;
        DROP TABLE IF EXISTS retrieval_queue;
        DROP TABLE IF EXISTS departments;
    )";

    char* errorMessage = nullptr;
    if (sqlite3_exec(outputDatabase, dropTablesSql, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        sqlite3_free(errorMessage);
        sqlite3_close(outputDatabase);
        return false;
    }

    const char* createTableFormat = R"(
        CREATE TABLE %s (
            retrieval_order INTEGER PRIMARY KEY,
            checkin_id INTEGER NOT NULL UNIQUE,
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
            priority INTEGER NOT NULL
        );
    )";

    for (const char* tableName : tableNames) {
        char createTableSql[1024];
        snprintf(createTableSql, sizeof(createTableSql), createTableFormat, tableName);
        if (sqlite3_exec(outputDatabase, createTableSql, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
            sqlite3_free(errorMessage);
            sqlite3_close(outputDatabase);
            return false;
        }
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
        );
    )";

    sqlite3_stmt* sourceStatement = nullptr;
    if (sqlite3_prepare_v2(sourceDatabase, query, -1, &sourceStatement, nullptr) != SQLITE_OK) {
        sqlite3_finalize(sourceStatement);
        sqlite3_close(outputDatabase);
        return false;
    }

    vector<RetrievalRecord> records;
    int sourceResult = SQLITE_ROW;
    while ((sourceResult = sqlite3_step(sourceStatement)) == SQLITE_ROW) {
        RetrievalRecord record;
        record.checkinId = sqlite3_column_int(sourceStatement, 0);
        record.patientId = sqlite3_column_int(sourceStatement, 1);
        record.patientName = reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 2));
        record.birthDate = sqlite3_column_type(sourceStatement, 3) == SQLITE_NULL
            ? ""
            : reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 3));
        record.age = sqlite3_column_int(sourceStatement, 4);
        record.gender = sqlite3_column_type(sourceStatement, 5) == SQLITE_NULL
            ? ""
            : reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 5));
        record.hometown = sqlite3_column_type(sourceStatement, 6) == SQLITE_NULL
            ? ""
            : reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 6));
        record.address = sqlite3_column_type(sourceStatement, 7) == SQLITE_NULL
            ? ""
            : reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 7));
        record.phone = sqlite3_column_type(sourceStatement, 8) == SQLITE_NULL
            ? ""
            : reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 8));
        record.department = reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 10));
        record.departmentOrder = departmentOrder(record.department);
        record.checkinTime = reinterpret_cast<const char*>(sqlite3_column_text(sourceStatement, 11));
        record.priority = sqlite3_column_int(sourceStatement, 12);
        records.push_back(record);
    }

    const bool sourceReadSuccessfully = sourceResult == SQLITE_DONE;
    sqlite3_finalize(sourceStatement);

    if (!sourceReadSuccessfully) {
        sqlite3_close(outputDatabase);
        return false;
    }

    vector<RetrievalRecord> buffer(records.size());
    if (!records.empty()) {
        mergeSort(records, buffer, 0, static_cast<int>(records.size()) - 1);
    }

    const char* insertFormat = "INSERT INTO %s VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* departmentStatements[10] = {};
    for (int index = 0; index < 10; ++index) {
        char insertSql[256];
        snprintf(insertSql, sizeof(insertSql), insertFormat, tableNames[index]);
        if (sqlite3_prepare_v2(outputDatabase, insertSql, -1, &departmentStatements[index], nullptr) != SQLITE_OK) {
            for (sqlite3_stmt* statement : departmentStatements) {
                sqlite3_finalize(statement);
            }
            sqlite3_close(outputDatabase);
            return false;
        }
    }

    bool success = true;
    int departmentOrders[10] = {};
    for (const RetrievalRecord& record : records) {
        sqlite3_stmt* departmentStatement = departmentStatements[record.departmentOrder - 1];
        sqlite3_reset(departmentStatement);
        sqlite3_clear_bindings(departmentStatement);
        sqlite3_bind_int(departmentStatement, 1, ++departmentOrders[record.departmentOrder - 1]);
        sqlite3_bind_int(departmentStatement, 2, record.checkinId);
        sqlite3_bind_int(departmentStatement, 3, record.patientId);
        sqlite3_bind_text(departmentStatement, 4, record.patientName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(departmentStatement, 5, record.birthDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(departmentStatement, 6, record.age);
        sqlite3_bind_text(departmentStatement, 7, record.gender.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(departmentStatement, 8, record.hometown.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(departmentStatement, 9, record.address.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(departmentStatement, 10, record.phone.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(departmentStatement, 11, record.departmentOrder);
        sqlite3_bind_text(departmentStatement, 12, record.department.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(departmentStatement, 13, record.checkinTime.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(departmentStatement, 14, record.priority);
        if (sqlite3_step(departmentStatement) != SQLITE_DONE) {
            success = false;
            break;
        }
    }

    for (sqlite3_stmt* statement : departmentStatements) {
        sqlite3_finalize(statement);
    }
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
