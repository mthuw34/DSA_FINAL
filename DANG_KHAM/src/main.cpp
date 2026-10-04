#include <filesystem>
#include <iostream>
#include <string>
#include <sqlite3.h>

// Chay tu thu muc goc du an, hoac truyen duong dan nguon va dich.
int main(int argc, char* argv[])
{
    if (argc != 1 && argc != 3)
    {
        std::cerr << "Cach dung: DangKham.exe [truyXuat.db dangKham.db]\n";
        return 1;
    }

    const std::filesystem::path source =
        argc == 3 ? argv[1] : "TRUY_XUAT_BENH_NHAN/db/truyXuat.db";

    const std::filesystem::path destination =
        argc == 3 ? argv[2] : "DANG_KHAM/db/dangKham.db";

    try
    {
        if (!std::filesystem::is_regular_file(source))
        {
            std::cerr << "Khong tim thay database nguon: "
                      << source << '\n';
            return 1;
        }

        if (std::filesystem::exists(destination) &&
            std::filesystem::equivalent(source, destination))
        {
            std::cerr << "Database nguon va dich phai khac nhau.\n";
            return 1;
        }

        if (!destination.parent_path().empty())
            std::filesystem::create_directories(destination.parent_path());
    }
    catch (const std::filesystem::filesystem_error& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }

    sqlite3* db = nullptr;

    const std::string destinationText = destination.string();

    if (sqlite3_open_v2(
            destinationText.c_str(),
            &db,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
            nullptr
        ) != SQLITE_OK)
    {
        std::cerr << "Loi mo database: "
                  << (db ? sqlite3_errmsg(db) : "loi SQLite")
                  << '\n';

        if (db != nullptr)
            sqlite3_close(db);

        return 1;
    }

    sqlite3_busy_timeout(db, 5000);

    // Mo database nguon chi doc.
    std::string sourceUri = "file:";
    const std::string absoluteSource =
        std::filesystem::absolute(source).generic_string();

    for (char c : absoluteSource)
    {
        if (c == '%') sourceUri += "%25";
        else if (c == '?') sourceUri += "%3F";
        else if (c == '#') sourceUri += "%23";
        else sourceUri += c;
    }

    sourceUri += "?mode=ro";

    sqlite3_stmt* attach = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        "ATTACH DATABASE ? AS source;",
        -1,
        &attach,
        nullptr
    );

    if (result == SQLITE_OK)
    {
        result = sqlite3_bind_text(
            attach,
            1,
            sourceUri.c_str(),
            -1,
            SQLITE_TRANSIENT
        );
    }

    if (result == SQLITE_OK)
        result = sqlite3_step(attach);

    sqlite3_finalize(attach);

    if (result != SQLITE_DONE)
    {
        std::cerr << "Loi doc database nguon: "
                  << sqlite3_errmsg(db)
                  << '\n';

        sqlite3_close(db);
        return 1;
    }

    const char* tables[] = {
        "queue_khoa_cap_cuu",
        "queue_khoa_noi",
        "queue_khoa_ngoai",
        "queue_khoa_tim_mach",
        "queue_khoa_nhi",
        "queue_khoa_san",
        "queue_khoa_tai_mui_hong",
        "queue_khoa_mat",
        "queue_khoa_da_lieu",
        "queue_khoa_than_kinh"
    };

    auto execute = [db](const std::string& sql)
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

        std::cerr << "Loi SQL: "
                  << (error ? error : sqlite3_errmsg(db))
                  << '\n';

        sqlite3_free(error);
        return false;
    };

    bool success = execute("BEGIN IMMEDIATE;");
    int total = 0;

    for (const char* table : tables)
    {
        if (!success)
            break;

        const std::string name = table;

        // DangKham la ban sao hien tai cua hang doi, nen tao lai schema
        // de tranh database cu chi co 5 cot.
        success = execute(
            "DROP TABLE IF EXISTS main." + name + ";"
        );

        if (!success)
            break;

        success = execute(
            "CREATE TABLE main." + name + R"( (
                retrieval_order INTEGER PRIMARY KEY,
                checkin_id INTEGER NOT NULL UNIQUE,
                patient_id INTEGER NOT NULL,
                department TEXT NOT NULL,
                checkin_time TEXT NOT NULL,
                base_priority INTEGER NOT NULL
                    CHECK(base_priority BETWEEN 1 AND 5),
                current_priority INTEGER NOT NULL
                    CHECK(current_priority BETWEEN 1 AND 5),
                last_update TEXT
            );)"
        );

        if (!success)
            break;

        success = execute(
            "INSERT INTO main." + name +
            " (retrieval_order, checkin_id, patient_id, department, "
            "checkin_time, base_priority, current_priority, last_update) "
            "SELECT retrieval_order, checkin_id, patient_id, department, "
            "checkin_time, base_priority, current_priority, last_update "
            "FROM source." + name +
            " ORDER BY retrieval_order;"
        );

        if (success)
            total += sqlite3_changes(db);
    }

    if (success)
        success = execute("DROP TABLE IF EXISTS main.dang_kham;");

    if (success)
        success = execute("COMMIT;");
    else
        execute("ROLLBACK;");

    // DETACH truoc khi dong ket noi de ket thuc sach database nguon.
    sqlite3_exec(db, "DETACH DATABASE source;", nullptr, nullptr, nullptr);
    sqlite3_close(db);

    if (!success)
        return 1;

    std::cout
        << "Da cap nhat 10 bang theo khoa: "
        << total
        << " dong, moi bang 8 cot.\n"
        << "Database: "
        << destination
        << '\n';

    return 0;
}
