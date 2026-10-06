#ifndef PRIORITY_STORAGE_H
#define PRIORITY_STORAGE_H

#include <iostream>
#include <string>
#include <vector>

#include <sqlite3.h>

namespace priority_storage
{

// Bản ghi trong database ưu tiên.
struct Record
{
    int checkinId = 0;
    int patientId = 0;
    std::string department;
    std::string checkinTime;
    int basePriority = 0;
    int currentPriority = 0;
    std::string lastUpdate;
    bool hasLastUpdate = false;
};

// Bản ghi từ database gốc.
struct SourceRecord
{
    int checkinId = 0;
    int patientId = 0;
    std::string department;
    std::string checkinTime;
    int basePriority = 0;
};

// Đọc cột văn bản, trả chuỗi rỗng nếu NULL.
inline std::string textColumn(sqlite3_stmt* stmt, int column)
{
    const unsigned char* text = sqlite3_column_text(stmt, column);
    return text ? reinterpret_cast<const char*>(text) : "";
}

// Chạy câu lệnh SQL và báo lỗi nếu thất bại.
inline bool execute(sqlite3* db, const char* sql, const char* message)
{
    char* error = nullptr;

    if (sqlite3_exec(db, sql, nullptr, nullptr, &error) == SQLITE_OK)
    {
        return true;
    }

    std::cerr << message << ": "
              << (error ? error : sqlite3_errmsg(db)) << '\n';

    sqlite3_free(error);
    return false;
}

// Đọc dữ liệu ưu tiên và thêm vào vector.
inline bool loadRecords(sqlite3* db, std::vector<Record>& records)
{
    const char* sql = R"(
        SELECT
            checkin_id,
            patient_id,
            department,
            checkin_time,
            base_priority,
            current_priority,
            last_update
        FROM priority_checkins;
    )";

    // Chuẩn bị câu lệnh.
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Loi doc priority.db: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    // Đọc lần lượt từng dòng.
    int result;

    while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        Record record;
        record.checkinId = sqlite3_column_int(stmt, 0);
        record.patientId = sqlite3_column_int(stmt, 1);
        record.department = textColumn(stmt, 2);
        record.checkinTime = textColumn(stmt, 3);
        record.basePriority = sqlite3_column_int(stmt, 4);
        record.currentPriority = sqlite3_column_int(stmt, 5);

        // Ghi nhận last_update có khác NULL không.
        record.hasLastUpdate = sqlite3_column_type(stmt, 6) != SQLITE_NULL;

        record.lastUpdate = textColumn(stmt, 6);
        records.push_back(record);
    }

    // Giải phóng câu lệnh.
    sqlite3_finalize(stmt);

    if (result != SQLITE_DONE)
    {
        std::cerr << "Loi doc cac ban ghi priority.db: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    return true;
}

// Đọc dữ liệu nguồn và thêm vào vector.
inline bool loadSourceRecords(sqlite3* db, std::vector<SourceRecord>& records)
{
    const char* sql = R"(
        SELECT
            checkin_id,
            patient_id,
            department,
            checkin_time,
            priority
        FROM checkins;
    )";

    // Chuẩn bị câu lệnh.
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Loi doc bang checkins trong hospital.db: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    // Đọc lần lượt từng dòng.
    int result;

    while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        SourceRecord record;

        record.checkinId = sqlite3_column_int(stmt, 0);
        record.patientId = sqlite3_column_int(stmt, 1);
        record.department = textColumn(stmt, 2);
        record.checkinTime = textColumn(stmt, 3);
        record.basePriority = sqlite3_column_int(stmt, 4);

        records.push_back(record);
    }

    sqlite3_finalize(stmt);

    if (result != SQLITE_DONE)
    {
        std::cerr << "Loi doc du lieu checkins: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }
    return true;
}

// Ghi lại toàn bộ danh sách; nơi gọi quản lý giao dịch.
inline bool replaceRecords(sqlite3* db, const std::vector<Record>& records)
{
    // Xóa các dòng cũ trong bảng.
    if (!execute(db,
            "DELETE FROM priority_checkins;",
            "Loi xoa du lieu cu de ghi lai priority.db"
        ))
    {
        return false;
    }

    const char* sql = R"(
        INSERT INTO priority_checkins
        (
            checkin_id,
            patient_id,
            department,
            checkin_time,
            base_priority,
            current_priority,
            last_update
        )
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Loi tao cau lenh ghi priority.db: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    // Ghi từng bản ghi trong vector.
    for (const Record& record : records)
    {
        // Đặt lại câu lệnh và xóa giá trị đã gán.
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);

        // Gán dữ liệu vào các dấu ?.
        sqlite3_bind_int(stmt, 1, record.checkinId);
        sqlite3_bind_int(stmt, 2, record.patientId);
        sqlite3_bind_text(stmt, 3, record.department.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, record.checkinTime.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, record.basePriority);
        sqlite3_bind_int(stmt, 6, record.currentPriority);

        // Giữ nguyên giá trị hoặc NULL của last_update.
        if (record.hasLastUpdate)
        {
            sqlite3_bind_text(stmt, 7, record.lastUpdate.c_str(), -1, SQLITE_TRANSIENT);
        }
        else sqlite3_bind_null(stmt, 7);

        // Thực hiện thêm bản ghi.
        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            std::cerr << "Loi ghi checkin_id " << record.checkinId
                      << " vao priority.db: "
                      << sqlite3_errmsg(db) << '\n';

            sqlite3_finalize(stmt);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    return true;
}
}

#endif