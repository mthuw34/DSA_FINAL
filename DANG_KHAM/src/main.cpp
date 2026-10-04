#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <sqlite3.h>

void NhapChanDoan(
    const std::string& idBenhNhan,
    std::string& chanDoan,
    std::string& donThuoc
);

static bool executeSql(
    sqlite3* db,
    const std::string& sql,
    const char* errorMessage
)
{
    char* error = nullptr;

    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error) == SQLITE_OK)
        return true;

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

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, tableName, -1, SQLITE_TRANSIENT);

    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);

    return exists;
}

static void hienThiDangKham(sqlite3* db)
{
    const char* sql = R"(
        SELECT
            checkin_id,
            patient_id,
            department,
            doctor_id,
            doctor_name,
            start_time,
            end_time
        FROM dang_kham
        ORDER BY datetime(start_time), checkin_id;
    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Loi doc danh sach dang kham: "
                  << sqlite3_errmsg(db) << '\n';
        return;
    }

    int count = 0;

    std::cout << "\n========== BENH NHAN DANG KHAM ==========\n";

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ++count;

        const char* department =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* doctorId =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* doctorName =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char* startTime =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* endTime =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

        std::cout
            << count
            << ". Check-in: " << sqlite3_column_int(stmt, 0)
            << " | BN: " << sqlite3_column_int(stmt, 1)
            << " | " << (department ? department : "")
            << " | BS: " << (doctorId ? doctorId : "")
            << " - " << (doctorName ? doctorName : "")
            << " | " << (startTime ? startTime : "")
            << " -> " << (endTime ? endTime : "")
            << '\n';
    }

    if (count == 0)
        std::cout << "Khong co benh nhan nao dang kham.\n";

    sqlite3_finalize(stmt);
}

static bool layBenhNhanDangKham(
    sqlite3* db,
    int checkinId,
    int& patientId
)
{
    const char* sql =
        "SELECT patient_id FROM dang_kham WHERE checkin_id = ?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, checkinId);

    const bool found = sqlite3_step(stmt) == SQLITE_ROW;

    if (found)
        patientId = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return found;
}

static bool luuChanDoan(
    sqlite3* db,
    int checkinId,
    int patientId,
    const std::string& chanDoan,
    const std::string& donThuoc
)
{
    const char* sql = R"(
        INSERT INTO chan_doan (
            checkin_id,
            patient_id,
            chan_doan,
            don_thuoc,
            updated_at
        )
        VALUES (?, ?, ?, ?, datetime('now', 'localtime'))
        ON CONFLICT(checkin_id)
        DO UPDATE SET
            patient_id = excluded.patient_id,
            chan_doan = excluded.chan_doan,
            don_thuoc = excluded.don_thuoc,
            updated_at = datetime('now', 'localtime');
    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Loi tao lenh luu chan doan: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    sqlite3_bind_int(stmt, 1, checkinId);
    sqlite3_bind_int(stmt, 2, patientId);
    sqlite3_bind_text(stmt, 3, chanDoan.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, donThuoc.c_str(), -1, SQLITE_TRANSIENT);

    const bool success = sqlite3_step(stmt) == SQLITE_DONE;

    if (!success)
    {
        std::cerr << "Luu chan doan that bai: "
                  << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
    return success;
}

static void hienThiChanDoan(sqlite3* db)
{
    const char* sql = R"(
        SELECT
            checkin_id,
            patient_id,
            chan_doan,
            don_thuoc,
            updated_at
        FROM chan_doan
        ORDER BY datetime(updated_at) DESC, checkin_id DESC;
    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Loi doc bang chan_doan: "
                  << sqlite3_errmsg(db) << '\n';
        return;
    }

    int count = 0;

    std::cout << "\n========== CHAN DOAN DA LUU ==========\n";

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ++count;

        const char* diagnosis =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* prescription =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* updatedAt =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

        std::cout
            << "Check-in: " << sqlite3_column_int(stmt, 0)
            << " | BN: " << sqlite3_column_int(stmt, 1)
            << "\nChan doan: " << (diagnosis ? diagnosis : "")
            << "\nDon thuoc: " << (prescription ? prescription : "")
            << "\nCap nhat: " << (updatedAt ? updatedAt : "")
            << "\n----------------------------------------\n";
    }

    if (count == 0)
        std::cout << "Chua co chan doan nao.\n";

    sqlite3_finalize(stmt);
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
        std::cerr << "Loi lien ket truyXuat.db: "
                  << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return 1;
    }

    if (!tableExists(db, "source", "ket_qua_kham"))
    {
        std::cerr
            << "Chua co bang ket_qua_kham trong truyXuat.db.\n"
            << "Hay chay SAP_XEP_BAC_SI truoc khi cap nhat DANG_KHAM.\n";

        sqlite3_exec(db, "DETACH DATABASE source;", nullptr, nullptr, nullptr);
        sqlite3_close(db);
        return 1;
    }

    bool success = executeSql(
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
            R"(
                CREATE TABLE IF NOT EXISTS chan_doan (
                    checkin_id INTEGER PRIMARY KEY,
                    patient_id INTEGER NOT NULL,
                    chan_doan TEXT NOT NULL,
                    don_thuoc TEXT,
                    updated_at TEXT NOT NULL
                        DEFAULT (datetime('now', 'localtime'))
                );
            )",
            "Loi tao bang chan_doan"
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
                ORDER BY datetime(start_time), checkin_id;
            )",
            "Loi cap nhat danh sach dang kham"
        );
    }

    const int total = success ? sqlite3_changes(db) : 0;

    if (success)
        success = executeSql(db, "COMMIT;", "Khong commit duoc giao dich");
    else
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);

    sqlite3_exec(db, "DETACH DATABASE source;", nullptr, nullptr, nullptr);

    if (!success)
    {
        sqlite3_close(db);
        return 1;
    }

    std::cout
        << "Da lien ket DANG_KHAM voi ket_qua_kham.\n"
        << "So benh nhan dang kham hien tai: "
        << total << '\n';

    while (true)
    {
        std::cout << "\n========== DANG KHAM ==========\n";
        std::cout << "1. Xem benh nhan dang kham\n";
        std::cout << "2. Nhap chan doan va don thuoc\n";
        std::cout << "3. Xem chan doan da luu\n";
        std::cout << "0. Thoat\n";
        std::cout << "Lua chon: ";

        int choice = -1;

        if (!(std::cin >> choice))
        {
            if (std::cin.eof() || std::cin.bad())
                break;

            std::cin.clear();
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );

            std::cout << "Lua chon khong hop le.\n";
            continue;
        }

        if (choice == 0)
            break;

        if (choice == 1)
        {
            hienThiDangKham(db);
        }
        else if (choice == 2)
        {
            std::cout << "Nhap check-in ID: ";

            int checkinId = 0;

            if (!(std::cin >> checkinId) || checkinId <= 0)
            {
                std::cin.clear();
                std::cin.ignore(
                    std::numeric_limits<std::streamsize>::max(),
                    '\n'
                );

                std::cout << "Check-in ID khong hop le.\n";
                continue;
            }

            int patientId = 0;

            if (!layBenhNhanDangKham(db, checkinId, patientId))
            {
                std::cout
                    << "Check-in ID "
                    << checkinId
                    << " khong nam trong danh sach dang kham.\n";

                continue;
            }

            std::string chanDoan;
            std::string donThuoc;

            NhapChanDoan(
                std::to_string(patientId),
                chanDoan,
                donThuoc
            );

            if (chanDoan.empty())
            {
                std::cout << "Chan doan khong duoc de trong.\n";
                continue;
            }

            if (luuChanDoan(
                    db,
                    checkinId,
                    patientId,
                    chanDoan,
                    donThuoc
                ))
            {
                std::cout << "Da luu chan doan vao dangKham.db.\n";
            }
        }
        else if (choice == 3)
        {
            hienThiChanDoan(db);
        }
        else
        {
            std::cout << "Lua chon khong hop le.\n";
        }
    }

    sqlite3_close(db);

    std::cout << "Da thoat DANG_KHAM.\n";
    return 0;
}
