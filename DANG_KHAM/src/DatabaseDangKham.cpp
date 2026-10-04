#include "DatabaseDangKham.h"

#include <filesystem>
#include <iostream>

using namespace std;

// Lấy chuỗi TEXT an toàn từ SQLite.
static string getText(
    sqlite3_stmt* stmt,
    int column
)
{
    const unsigned char* text =
        sqlite3_column_text(stmt, column);

    return text
        ? reinterpret_cast<const char*>(text)
        : "";
}

DatabaseDangKham::~DatabaseDangKham()
{
    dong();
}

// Thực thi một câu lệnh SQL không cần trả về dòng dữ liệu.
bool DatabaseDangKham::executeSql(
    const string& sql,
    const char* errorMessage
)
{
    char* error = nullptr;

    if (sqlite3_exec(
            db,
            sql.c_str(),
            nullptr,
            nullptr,
            &error
        ) == SQLITE_OK)
    {
        return true;
    }

    cerr
        << errorMessage
        << ": "
        << (error ? error : sqlite3_errmsg(db))
        << '\n';

    sqlite3_free(error);
    return false;
}

// Kiểm tra một cột đã tồn tại trong bảng hay chưa.
bool DatabaseDangKham::columnExists(
    const char* table,
    const char* column
)
{
    const string sql =
        "PRAGMA table_info(" +
        string(table) +
        ");";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            sql.c_str(),
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }

    bool found = false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (getText(stmt, 1) == column)
        {
            found = true;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return found;
}

// Bổ sung cột vào bảng nếu database cũ chưa có cột đó.
bool DatabaseDangKham::addColumnIfMissing(
    const char* table,
    const char* column,
    const char* definition
)
{
    if (columnExists(table, column))
        return true;

    const string sql =
        "ALTER TABLE " +
        string(table) +
        " ADD COLUMN " +
        column +
        " " +
        definition +
        ";";

    return executeSql(
        sql,
        "Loi bo sung cot"
    );
}

// Kiểm tra bảng nguồn có tồn tại trong database được liên kết.
bool DatabaseDangKham::sourceTableExists(
    const char* tableName
)
{
    const char* sql = R"(
        SELECT 1
        FROM source.sqlite_master
        WHERE type = 'table'
          AND name = ?
        LIMIT 1;
    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(
        stmt,
        1,
        tableName,
        -1,
        SQLITE_TRANSIENT
    );

    const bool exists =
        sqlite3_step(stmt) == SQLITE_ROW;

    sqlite3_finalize(stmt);
    return exists;
}

// Mở dangKham.db và liên kết database nguồn.
bool DatabaseDangKham::mo(
    const string& sourcePath,
    const string& destinationPath
)
{
    if (sqlite3_open_v2(
            destinationPath.c_str(),
            &db,
            SQLITE_OPEN_READWRITE |
            SQLITE_OPEN_CREATE,
            nullptr
        ) != SQLITE_OK)
    {
        cerr
            << "Khong mo duoc dangKham.db: "
            << (db ? sqlite3_errmsg(db) : "loi SQLite")
            << '\n';

        dong();
        return false;
    }

    sqlite3_busy_timeout(db, 5000);

    const string absoluteSource =
        filesystem::absolute(sourcePath)
            .string();

    sqlite3_stmt* stmt = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        "ATTACH DATABASE ? AS source;",
        -1,
        &stmt,
        nullptr
    );

    if (result == SQLITE_OK)
    {
        result = sqlite3_bind_text(
            stmt,
            1,
            absoluteSource.c_str(),
            -1,
            SQLITE_TRANSIENT
        );
    }

    if (result == SQLITE_OK)
        result = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (result != SQLITE_DONE)
    {
        cerr
            << "Loi lien ket truyXuat.db: "
            << sqlite3_errmsg(db)
            << '\n';

        dong();
        return false;
    }

    return true;
}

// Tạo và cập nhật cấu trúc bảng dang_kham.
bool DatabaseDangKham::taoCauTruc()
{
    if (!executeSql(
            R"(
                CREATE TABLE IF NOT EXISTS dang_kham (
                    checkin_id INTEGER PRIMARY KEY,
                    patient_id INTEGER NOT NULL,
                    department TEXT NOT NULL,
                    checkin_time TEXT NOT NULL,
                    doctor_id TEXT,
                    doctor_name TEXT,
                    start_time TEXT,
                    end_time TEXT,
                    chan_doan TEXT,
                    don_thuoc TEXT,
                    loi_nhac_bac_si TEXT,
                    updated_at TEXT
                );
            )",
            "Loi tao bang dang_kham"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "checkin_time",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "doctor_id",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "doctor_name",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "start_time",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "end_time",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "chan_doan",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "don_thuoc",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "loi_nhac_bac_si",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "updated_at",
            "TEXT"
        ))
    {
        return false;
    }

    // ket_qua_kham chỉ có giờ bắt đầu khám; dùng làm mốc thay thế
    // cho bản ghi cũ thiếu giờ check-in, không sửa giờ đã được lưu.
    return executeSql(
        "UPDATE dang_kham SET checkin_time = start_time "
        "WHERE checkin_time IS NULL;",
        "Loi bo sung thoi gian check-in"
    );
}

// Nhận kết quả phân bác sĩ và đồng bộ vào dangKham.db.
bool DatabaseDangKham::dongBoTuXepBacSi()
{
    if (!sourceTableExists("ket_qua_kham"))
    {
        cerr
            << "Chua co bang ket_qua_kham trong truyXuat.db.\n"
            << "Hay chay SAP_XEP_BAC_SI truoc.\n";

        return false;
    }

    return executeSql(
        R"(
            INSERT INTO dang_kham (
                checkin_id,
                patient_id,
                department,
                checkin_time,
                doctor_id,
                doctor_name,
                start_time,
                updated_at
            )
            SELECT
                checkin_id,
                patient_id,
                khoa_bac_si,
                start_time,
                doctor_id,
                doctor_name,
                start_time,
                datetime('now', 'localtime')
            FROM source.ket_qua_kham
            WHERE Status = 'DA_XEP_BAC_SI'
              AND datetime(start_time) <= datetime('now', 'localtime')
              -- Chỉ nhận ca mới trong lịch dự kiến. Ca đã nhận vẫn chờ
              -- bác sĩ kết thúc thực tế, không tự ghi end_time.
              AND (datetime(end_time) > datetime('now', 'localtime')
                   OR EXISTS (
                       SELECT 1 FROM dang_kham
                       WHERE dang_kham.checkin_id = source.ket_qua_kham.checkin_id
                   ))
            ON CONFLICT(checkin_id)
            DO UPDATE SET
                patient_id = excluded.patient_id,
                department = excluded.department,
                doctor_id = excluded.doctor_id,
                doctor_name = excluded.doctor_name,
                start_time = excluded.start_time
            WHERE dang_kham.end_time IS NULL;
        )",
        "Loi dong bo benh nhan dang kham"
    );
}

// Ngắt liên kết database nguồn và đóng kết nối SQLite.
void DatabaseDangKham::dong()
{
    if (db == nullptr)
        return;

    sqlite3_exec(
        db,
        "DETACH DATABASE source;",
        nullptr,
        nullptr,
        nullptr
    );

    sqlite3_close(db);
    db = nullptr;
}

// Trả về kết nối SQLite để các lớp nghiệp vụ sử dụng.
sqlite3* DatabaseDangKham::get() const
{
    return db;
}
