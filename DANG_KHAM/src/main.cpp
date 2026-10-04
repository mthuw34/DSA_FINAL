// Minh Thu
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

static std::string getText(sqlite3_stmt* stmt, int column)
{
    const unsigned char* text = sqlite3_column_text(stmt, column);
    return text ? reinterpret_cast<const char*>(text) : "";
}

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

static bool columnExists(
    sqlite3* db,
    const char* table,
    const char* column
)
{
    const std::string sql =
        "PRAGMA table_info(" + std::string(table) + ");";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return false;

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

static bool taoBangDangKham(sqlite3* db)
{
    if (!executeSql(
            db,
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

    if (!addColumnIfMissing(db, "dang_kham", "doctor_id", "TEXT"))
        return false;

    if (!addColumnIfMissing(db, "dang_kham", "doctor_name", "TEXT"))
        return false;

    if (!addColumnIfMissing(db, "dang_kham", "start_time", "TEXT"))
        return false;

    if (!addColumnIfMissing(db, "dang_kham", "end_time", "TEXT"))
        return false;

    if (!addColumnIfMissing(db, "dang_kham", "chan_doan", "TEXT"))
        return false;

    if (!addColumnIfMissing(db, "dang_kham", "don_thuoc", "TEXT"))
        return false;

    if (!addColumnIfMissing(db, "dang_kham", "updated_at", "TEXT"))
        return false;

    return true;
}

static bool taoViewHangDoiNguon(sqlite3* db)
{
    const char* sql = R"(
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
    )";

    return executeSql(
        db,
        sql,
        "Loi tao view hang doi"
    );
}

static void hienThiHangDoi(sqlite3* db)
{
    const char* sql = R"(
        SELECT
            retrieval_order,
            checkin_id,
            patient_id,
            department,
            checkin_time
        FROM hang_doi_nguon
        ORDER BY department, retrieval_order, checkin_id;
    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Loi doc hang doi: "
                  << sqlite3_errmsg(db) << '\n';
        return;
    }

    std::cout << "\n========== HANG DOI BENH NHAN ==========\n";

    int count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ++count;

        std::cout
            << count
            << ". STT: " << sqlite3_column_int(stmt, 0)
            << " | Check-in: " << sqlite3_column_int(stmt, 1)
            << " | BN: " << sqlite3_column_int(stmt, 2)
            << " | " << getText(stmt, 3)
            << " | " << getText(stmt, 4)
            << '\n';
    }

    if (count == 0)
        std::cout << "Hang doi rong.\n";

    sqlite3_finalize(stmt);
}

static bool layBenhNhanTuHangDoi(
    sqlite3* db,
    int checkinId,
    int& patientId,
    std::string& department,
    std::string& checkinTime
)
{
    const char* sql = R"(
        SELECT
            patient_id,
            department,
            checkin_time
        FROM hang_doi_nguon
        WHERE checkin_id = ?
        LIMIT 1;
    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, checkinId);

    const bool found = sqlite3_step(stmt) == SQLITE_ROW;

    if (found)
    {
        patientId = sqlite3_column_int(stmt, 0);
        department = getText(stmt, 1);
        checkinTime = getText(stmt, 2);
    }

    sqlite3_finalize(stmt);
    return found;
}

static bool daCoPhienKham(sqlite3* db, int checkinId)
{
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            "SELECT 1 FROM dang_kham WHERE checkin_id = ? LIMIT 1;",
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, checkinId);

    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;

    sqlite3_finalize(stmt);
    return exists;
}

static void batDauKham(sqlite3* db)
{
    std::cout << "Nhap check-in ID bat dau kham: ";

    int checkinId = 0;

    if (!(std::cin >> checkinId) || checkinId <= 0)
    {
        std::cin.clear();
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );

        std::cout << "Check-in ID khong hop le.\n";
        return;
    }

    int patientId = 0;
    std::string department;
    std::string checkinTime;

    if (!layBenhNhanTuHangDoi(
            db,
            checkinId,
            patientId,
            department,
            checkinTime
        ))
    {
        std::cout << "Khong tim thay check-in ID trong hang doi.\n";
        return;
    }

    if (daCoPhienKham(db, checkinId))
    {
        std::cout << "Check-in nay da co phien kham trong dangKham.db.\n";
        return;
    }

    std::string doctorId;
    std::string doctorName;

    std::cout << "Nhap ma bac si: ";
    std::getline(std::cin >> std::ws, doctorId);

    std::cout << "Nhap ten bac si: ";
    std::getline(std::cin, doctorName);

    if (doctorId.empty() || doctorName.empty())
    {
        std::cout << "Thong tin bac si khong duoc de trong.\n";
        return;
    }

    const char* sql = R"(
        INSERT INTO dang_kham (
            checkin_id,
            patient_id,
            department,
            checkin_time,
            doctor_id,
            doctor_name,
            start_time,
            end_time,
            chan_doan,
            don_thuoc,
            updated_at
        )
        VALUES (
            ?, ?, ?, ?, ?, ?,
            datetime('now', 'localtime'),
            NULL,
            NULL,
            NULL,
            datetime('now', 'localtime')
        );
    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Loi tao phien kham: "
                  << sqlite3_errmsg(db) << '\n';
        return;
    }

    sqlite3_bind_int(stmt, 1, checkinId);
    sqlite3_bind_int(stmt, 2, patientId);
    sqlite3_bind_text(stmt, 3, department.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, checkinTime.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, doctorId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, doctorName.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
        std::cout << "Da bat dau kham.\n";
    }
    else
    {
        std::cerr << "Bat dau kham that bai: "
                  << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
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
            chan_doan
        FROM dang_kham
        WHERE end_time IS NULL
        ORDER BY datetime(start_time), checkin_id;
    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Loi doc dang_kham: "
                  << sqlite3_errmsg(db) << '\n';
        return;
    }

    std::cout << "\n========== BENH NHAN DANG KHAM ==========\n";

    int count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ++count;

        std::cout
            << count
            << ". Check-in: " << sqlite3_column_int(stmt, 0)
            << " | BN: " << sqlite3_column_int(stmt, 1)
            << " | " << getText(stmt, 2)
            << " | BS: " << getText(stmt, 3)
            << " - " << getText(stmt, 4)
            << " | Bat dau: " << getText(stmt, 5)
            << " | Chan doan: "
            << (sqlite3_column_type(stmt, 6) == SQLITE_NULL
                    ? "Chua nhap"
                    : getText(stmt, 6))
            << '\n';
    }

    if (count == 0)
        std::cout << "Khong co benh nhan nao dang kham.\n";

    sqlite3_finalize(stmt);
}

static bool layPhienDangKham(
    sqlite3* db,
    int checkinId,
    int& patientId
)
{
    const char* sql = R"(
        SELECT patient_id
        FROM dang_kham
        WHERE checkin_id = ?
          AND end_time IS NULL;
    )";

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

static void nhapChanDoan(sqlite3* db)
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
        return;
    }

    int patientId = 0;

    if (!layPhienDangKham(db, checkinId, patientId))
    {
        std::cout << "Benh nhan khong o trang thai dang kham.\n";
        return;
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
        return;
    }

    const char* sql = R"(
        UPDATE dang_kham
        SET
            chan_doan = ?,
            don_thuoc = ?,
            updated_at = datetime('now', 'localtime')
        WHERE checkin_id = ?
          AND end_time IS NULL;
    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Loi tao lenh cap nhat chan doan: "
                  << sqlite3_errmsg(db) << '\n';
        return;
    }

    sqlite3_bind_text(stmt, 1, chanDoan.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, donThuoc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, checkinId);

    if (sqlite3_step(stmt) == SQLITE_DONE &&
        sqlite3_changes(db) > 0)
    {
        std::cout << "Da luu chan doan va don thuoc.\n";
    }
    else
    {
        std::cerr << "Khong cap nhat duoc chan doan.\n";
    }

    sqlite3_finalize(stmt);
}

static void ketThucKham(sqlite3* db)
{
    std::cout << "Nhap check-in ID ket thuc kham: ";

    int checkinId = 0;

    if (!(std::cin >> checkinId) || checkinId <= 0)
    {
        std::cin.clear();
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );

        std::cout << "Check-in ID khong hop le.\n";
        return;
    }

    const char* sql = R"(
        UPDATE dang_kham
        SET
            end_time = datetime('now', 'localtime'),
            updated_at = datetime('now', 'localtime')
        WHERE checkin_id = ?
          AND end_time IS NULL;
    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Loi tao lenh ket thuc kham: "
                  << sqlite3_errmsg(db) << '\n';
        return;
    }

    sqlite3_bind_int(stmt, 1, checkinId);

    if (sqlite3_step(stmt) == SQLITE_DONE &&
        sqlite3_changes(db) > 0)
    {
        std::cout << "Da ket thuc phien kham.\n";
    }
    else
    {
        std::cout << "Khong tim thay phien dang kham phu hop.\n";
    }

    sqlite3_finalize(stmt);
}

static void hienThiLichSu(sqlite3* db)
{
    const char* sql = R"(
        SELECT
            checkin_id,
            patient_id,
            department,
            doctor_id,
            doctor_name,
            start_time,
            end_time,
            chan_doan,
            don_thuoc
        FROM dang_kham
        ORDER BY datetime(start_time) DESC, checkin_id DESC;
    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Loi doc lich su: "
                  << sqlite3_errmsg(db) << '\n';
        return;
    }

    std::cout << "\n========== LICH SU KHAM ==========\n";

    int count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ++count;

        std::cout
            << "Check-in: " << sqlite3_column_int(stmt, 0)
            << " | BN: " << sqlite3_column_int(stmt, 1)
            << " | " << getText(stmt, 2)
            << "\nBac si: " << getText(stmt, 3)
            << " - " << getText(stmt, 4)
            << "\nBat dau: " << getText(stmt, 5)
            << "\nKet thuc: "
            << (sqlite3_column_type(stmt, 6) == SQLITE_NULL
                    ? "Dang kham"
                    : getText(stmt, 6))
            << "\nChan doan: "
            << (sqlite3_column_type(stmt, 7) == SQLITE_NULL
                    ? "Chua co"
                    : getText(stmt, 7))
            << "\nDon thuoc: "
            << (sqlite3_column_type(stmt, 8) == SQLITE_NULL
                    ? "Chua co"
                    : getText(stmt, 8))
            << "\n----------------------------------------\n";
    }

    if (count == 0)
        std::cout << "Chua co lich su kham.\n";

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
    const std::string destinationText = destination.string();

    if (sqlite3_open_v2(
            destinationText.c_str(),
            &db,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
            nullptr
        ) != SQLITE_OK)
    {
        std::cerr << "Khong mo duoc dangKham.db: "
                  << (db ? sqlite3_errmsg(db) : "loi SQLite")
                  << '\n';

        if (db != nullptr)
            sqlite3_close(db);

        return 1;
    }

    sqlite3_busy_timeout(db, 5000);

    if (!taoBangDangKham(db))
    {
        sqlite3_close(db);
        return 1;
    }

    const std::string sourcePath =
        std::filesystem::absolute(source).string();

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
            sourcePath.c_str(),
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

    if (!taoViewHangDoiNguon(db))
    {
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

    while (true)
    {
        std::cout << "\n========== DANG KHAM ==========\n";
        std::cout << "1. Xem hang doi benh nhan\n";
        std::cout << "2. Bat dau kham\n";
        std::cout << "3. Xem benh nhan dang kham\n";
        std::cout << "4. Nhap chan doan va don thuoc\n";
        std::cout << "5. Ket thuc kham\n";
        std::cout << "6. Xem lich su kham\n";
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
            hienThiHangDoi(db);
        else if (choice == 2)
            batDauKham(db);
        else if (choice == 3)
            hienThiDangKham(db);
        else if (choice == 4)
            nhapChanDoan(db);
        else if (choice == 5)
            ketThucKham(db);
        else if (choice == 6)
            hienThiLichSu(db);
        else
            std::cout << "Lua chon khong hop le.\n";
    }

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

    std::cout << "Da thoat DANG_KHAM.\n";
    return 0;
}
