#include <filesystem>
#include <iostream>
#include <string>
#include <sqlite3.h>

static bool executeSql(
    sqlite3* db,
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

    std::cerr << errorMessage << ": "
              << (error ? error : sqlite3_errmsg(db))
              << '\n';

    sqlite3_free(error);
    return false;
}

static bool tableExists(
    sqlite3* db,
    const char* databaseName,
    const char* tableName
)
{
    const std::string sql =
        "SELECT 1 FROM " + std::string(databaseName) +
        ".sqlite_master WHERE type = 'table' AND name = ? LIMIT 1;";

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

int main(int argc, char* argv[])
{
    if (argc != 1 && argc != 3)
    {
        std::cerr
            << "Cach dung: DangKham.exe "
            << "[truyXuat.db dangKham.db]\n";

        return 1;
    }

    const std::filesystem::path source =
        argc == 3
            ? argv[1]
            : "TRUY_XUAT_BENH_NHAN/db/truyXuat.db";

    const std::filesystem::path destination =
        argc == 3
            ? argv[2]
            : "DANG_KHAM/db/dangKham.db";

    try
    {
        if (!std::filesystem::is_regular_file(source))
        {
            std::cerr
                << "Khong tim thay database nguon: "
                << source
                << '\n';

            return 1;
        }

        if (std::filesystem::exists(destination) &&
            std::filesystem::equivalent(source, destination))
        {
            std::cerr
                << "Database nguon va dich phai khac nhau.\n";

            return 1;
        }

        if (!destination.parent_path().empty())
        {
            std::filesystem::create_directories(
                destination.parent_path()
            );
        }
    }
    catch (const std::filesystem::filesystem_error& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }

    sqlite3* db = nullptr;

    const std::string destinationText =
        destination.string();

    if (sqlite3_open_v2(
            destinationText.c_str(),
            &db,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
            nullptr
        ) != SQLITE_OK)
    {
        std::cerr
            << "Loi mo database: "
            << (db ? sqlite3_errmsg(db) : "loi SQLite")
            << '\n';

        if (db != nullptr)
            sqlite3_close(db);

        return 1;
    }

    sqlite3_busy_timeout(db, 5000);

    // Gan truyXuat.db vao ket noi hien tai o che do chi doc.
    std::string sourceUri = "file:";

    const std::string absoluteSource =
        std::filesystem::absolute(source)
            .generic_string();

    for (char c : absoluteSource)
    {
        if (c == '%') sourceUri += "%25";
        else if (c == '?') sourceUri += "%3F";
        else if (c == '#') sourceUri += "%23";
        else sourceUri += c;
    }

    sourceUri += "?mode=ro";

    sqlite3_stmt* attachStmt = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        "ATTACH DATABASE ? AS source;",
        -1,
        &attachStmt,
        nullptr
    );

    if (result == SQLITE_OK)
    {
        result = sqlite3_bind_text(
            attachStmt,
            1,
            sourceUri.c_str(),
            -1,
            SQLITE_TRANSIENT
        );
    }

    if (result == SQLITE_OK)
        result = sqlite3_step(attachStmt);

    sqlite3_finalize(attachStmt);

    if (result != SQLITE_DONE)
    {
        std::cerr
            << "Loi lien ket truyXuat.db: "
            << sqlite3_errmsg(db)
            << '\n';

        sqlite3_close(db);
        return 1;
    }

    // DANG_KHAM chi lay ket qua sau khi SAP_XEP_BAC_SI
    // da phan bac si va tao bang ket_qua_kham.
    if (!tableExists(
            db,
            "source",
            "ket_qua_kham"
        ))
    {
        std::cerr
            << "Chua co bang ket_qua_kham trong truyXuat.db.\n"
            << "Hay chay SAP_XEP_BAC_SI truoc khi cap nhat DANG_KHAM.\n";

        sqlite3_exec(
            db,
            "DETACH DATABASE source;",
            nullptr,
            nullptr,
            nullptr
        );

        sqlite3_close(db);
        return 1;
    }

    bool success =
        executeSql(
            db,
            "BEGIN IMMEDIATE;",
            "Khong bat dau duoc giao dich"
        );

    if (success)
    {
        success = executeSql(
            db,
            R"(
                CREATE TABLE IF NOT EXISTS dang_kham (
                    checkin_id INTEGER PRIMARY KEY,
                    patient_id INTEGER NOT NULL,
                    department TEXT NOT NULL,
                    doctor_id TEXT NOT NULL,
                    doctor_name TEXT NOT NULL,
                    start_time TEXT NOT NULL,
                    end_time TEXT NOT NULL
                );
            )",
            "Loi tao bang dang_kham"
        );
    }

    if (success)
    {
        success = executeSql(
            db,
            "DELETE FROM dang_kham;",
            "Loi xoa snapshot dang_kham cu"
        );
    }

    if (success)
    {
        success = executeSql(
            db,
            R"(
                INSERT INTO dang_kham (
                    checkin_id,
                    patient_id,
                    department,
                    doctor_id,
                    doctor_name,
                    start_time,
                    end_time
                )
                SELECT
                    checkin_id,
                    patient_id,
                    khoa_bac_si,
                    doctor_id,
                    doctor_name,
                    start_time,
                    end_time
                FROM source.ket_qua_kham
                WHERE Status = 'DA_XEP_BAC_SI'
                  AND datetime(start_time) <= datetime('now', 'localtime')
                  AND datetime(end_time) > datetime('now', 'localtime')
                ORDER BY
                    datetime(start_time) ASC,
                    checkin_id ASC;
            )",
            "Loi cap nhat danh sach dang kham"
        );
    }

    const int total =
        success
            ? sqlite3_changes(db)
            : 0;

    if (success)
    {
        success = executeSql(
            db,
            "COMMIT;",
            "Khong commit duoc giao dich"
        );
    }
    else
    {
        sqlite3_exec(
            db,
            "ROLLBACK;",
            nullptr,
            nullptr,
            nullptr
        );
    }

    sqlite3_exec(
        db,
        "DETACH DATABASE source;",
        nullptr,
        nullptr,
        nullptr
    );

    sqlite3_close(db);

    if (!success)
        return 1;

    std::cout
        << "Da lien ket DANG_KHAM voi ket_qua_kham.\n"
        << "So benh nhan dang kham hien tai: "
        << total
        << '\n'
        << "Database: "
        << destination
        << '\n';

    return 0;
}
