#include "DatabaseDangKham.h"

#include <filesystem>
#include <iostream>

static std::string getText(
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

bool DatabaseDangKham::executeSql(
    const std::string& sql,
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

    std::cerr
        << errorMessage
        << ": "
        << (error ? error : sqlite3_errmsg(db))
        << '\n';

    sqlite3_free(error);
    return false;
}

bool DatabaseDangKham::columnExists(
    const char* table,
    const char* column
)
{
    const std::string sql =
        "PRAGMA table_info(" +
        std::string(table) +
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

bool DatabaseDangKham::addColumnIfMissing(
    const char* table,
    const char* column,
    const char* definition
)
{
    if (columnExists(table, column))
        return true;

    const std::string sql =
        "ALTER TABLE " +
        std::string(table) +
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

bool DatabaseDangKham::mo(
    const std::string& sourcePath,
    const std::string& destinationPath
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
        std::cerr
            << "Khong mo duoc dangKham.db: "
            << (db ? sqlite3_errmsg(db) : "loi SQLite")
            << '\n';

        dong();
        return false;
    }

    sqlite3_busy_timeout(db, 5000);

    const std::string absoluteSource =
        std::filesystem::absolute(sourcePath)
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
        std::cerr
            << "Loi lien ket truyXuat.db: "
            << sqlite3_errmsg(db)
            << '\n';

        dong();
        return false;
    }

    return true;
}

bool DatabaseDangKham::taoCauTruc()
{
    if (!executeSql(
            R"(
                CREATE TABLE IF NOT EXISTS dang_kham (
                    checkin_id INTEGER PRIMARY KEY,
                    patient_id INTEGER NOT NULL,
                    department TEXT NOT NULL,
                    checkin_time TEXT NOT NULL
                );
            )",
            "Loi tao bang dang_kham"
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
            "updated_at",
            "TEXT"
        ))
    {
        return false;
    }

    return true;
}

bool DatabaseDangKham::taoViewHangDoiNguon()
{
    return executeSql(
        R"(
            DROP VIEW IF EXISTS temp.hang_doi_nguon;

            CREATE TEMP VIEW hang_doi_nguon AS

            SELECT retrieval_order, checkin_id, patient_id, department, checkin_time
            FROM source.queue_khoa_cap_cuu

            UNION ALL
            SELECT retrieval_order, checkin_id, patient_id, department, checkin_time
            FROM source.queue_khoa_noi

            UNION ALL
            SELECT retrieval_order, checkin_id, patient_id, department, checkin_time
            FROM source.queue_khoa_ngoai

            UNION ALL
            SELECT retrieval_order, checkin_id, patient_id, department, checkin_time
            FROM source.queue_khoa_tim_mach

            UNION ALL
            SELECT retrieval_order, checkin_id, patient_id, department, checkin_time
            FROM source.queue_khoa_nhi

            UNION ALL
            SELECT retrieval_order, checkin_id, patient_id, department, checkin_time
            FROM source.queue_khoa_san

            UNION ALL
            SELECT retrieval_order, checkin_id, patient_id, department, checkin_time
            FROM source.queue_khoa_tai_mui_hong

            UNION ALL
            SELECT retrieval_order, checkin_id, patient_id, department, checkin_time
            FROM source.queue_khoa_mat

            UNION ALL
            SELECT retrieval_order, checkin_id, patient_id, department, checkin_time
            FROM source.queue_khoa_da_lieu

            UNION ALL
            SELECT retrieval_order, checkin_id, patient_id, department, checkin_time
            FROM source.queue_khoa_than_kinh;
        )",
        "Loi tao view hang doi"
    );
}

void DatabaseDangKham::dong()
{
    if (db == nullptr)
        return;

    sqlite3_exec(
        db,
        "DROP VIEW IF EXISTS temp.hang_doi_nguon;",
        nullptr,
        nullptr,
        nullptr
    );

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

sqlite3* DatabaseDangKham::get() const
{
    return db;
}
