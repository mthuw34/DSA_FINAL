#include "DangKhamManager.h"

#include <iostream>
#include <limits>

#include "NhapChanDoan.h"

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

bool DangKhamManager::khoiDong(
    const std::string& sourcePath,
    const std::string& destinationPath
)
{
    if (!database.mo(
            sourcePath,
            destinationPath
        ))
    {
        return false;
    }

    if (!database.taoCauTruc())
        return false;

    if (!database.taoViewHangDoiNguon())
        return false;

    return true;
}

void DangKhamManager::hienThiHangDoi()
{
    sqlite3* db = database.get();

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

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        std::cerr
            << "Loi doc hang doi: "
            << sqlite3_errmsg(db)
            << '\n';

        return;
    }

    std::cout
        << "\n========== HANG DOI BENH NHAN ==========\n";

    int count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ++count;

        std::cout
            << count
            << ". STT: "
            << sqlite3_column_int(stmt, 0)
            << " | Check-in: "
            << sqlite3_column_int(stmt, 1)
            << " | BN: "
            << sqlite3_column_int(stmt, 2)
            << " | "
            << getText(stmt, 3)
            << " | "
            << getText(stmt, 4)
            << '\n';
    }

    if (count == 0)
        std::cout << "Hang doi rong.\n";

    sqlite3_finalize(stmt);
}

bool DangKhamManager::layBenhNhanTuHangDoi(
    int checkinId,
    int& patientId,
    std::string& department,
    std::string& checkinTime
)
{
    sqlite3* db = database.get();

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

    sqlite3_bind_int(
        stmt,
        1,
        checkinId
    );

    const bool found =
        sqlite3_step(stmt) == SQLITE_ROW;

    if (found)
    {
        patientId =
            sqlite3_column_int(stmt, 0);

        department =
            getText(stmt, 1);

        checkinTime =
            getText(stmt, 2);
    }

    sqlite3_finalize(stmt);
    return found;
}

bool DangKhamManager::daCoPhienKham(
    int checkinId
)
{
    sqlite3* db = database.get();
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            "SELECT 1 FROM dang_kham "
            "WHERE checkin_id = ? LIMIT 1;",
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(
        stmt,
        1,
        checkinId
    );

    const bool exists =
        sqlite3_step(stmt) == SQLITE_ROW;

    sqlite3_finalize(stmt);
    return exists;
}

void DangKhamManager::batDauKham()
{
    sqlite3* db = database.get();

    std::cout
        << "Nhap check-in ID bat dau kham: ";

    int checkinId = 0;

    if (!(std::cin >> checkinId) ||
        checkinId <= 0)
    {
        std::cin.clear();

        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );

        std::cout
            << "Check-in ID khong hop le.\n";

        return;
    }

    int patientId = 0;
    std::string department;
    std::string checkinTime;

    if (!layBenhNhanTuHangDoi(
            checkinId,
            patientId,
            department,
            checkinTime
        ))
    {
        std::cout
            << "Khong tim thay check-in ID "
            << "trong hang doi.\n";

        return;
    }

    if (daCoPhienKham(checkinId))
    {
        std::cout
            << "Check-in nay da co phien kham "
            << "trong dangKham.db.\n";

        return;
    }

    std::string doctorId;
    std::string doctorName;

    std::cout << "Nhap ma bac si: ";
    std::getline(
        std::cin >> std::ws,
        doctorId
    );

    std::cout << "Nhap ten bac si: ";
    std::getline(
        std::cin,
        doctorName
    );

    if (doctorId.empty() ||
        doctorName.empty())
    {
        std::cout
            << "Thong tin bac si "
            << "khong duoc de trong.\n";

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

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        std::cerr
            << "Loi tao phien kham: "
            << sqlite3_errmsg(db)
            << '\n';

        return;
    }

    sqlite3_bind_int(
        stmt,
        1,
        checkinId
    );

    sqlite3_bind_int(
        stmt,
        2,
        patientId
    );

    sqlite3_bind_text(
        stmt,
        3,
        department.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        stmt,
        4,
        checkinTime.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        stmt,
        5,
        doctorId.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        stmt,
        6,
        doctorName.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    if (sqlite3_step(stmt) == SQLITE_DONE)
        std::cout << "Da bat dau kham.\n";
    else
        std::cerr
            << "Bat dau kham that bai: "
            << sqlite3_errmsg(db)
            << '\n';

    sqlite3_finalize(stmt);
}

void DangKhamManager::hienThiDangKham()
{
    sqlite3* db = database.get();

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

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        std::cerr
            << "Loi doc dang_kham: "
            << sqlite3_errmsg(db)
            << '\n';

        return;
    }

    std::cout
        << "\n========== BENH NHAN DANG KHAM ==========\n";

    int count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ++count;

        std::cout
            << count
            << ". Check-in: "
            << sqlite3_column_int(stmt, 0)
            << " | BN: "
            << sqlite3_column_int(stmt, 1)
            << " | "
            << getText(stmt, 2)
            << " | BS: "
            << getText(stmt, 3)
            << " - "
            << getText(stmt, 4)
            << " | Bat dau: "
            << getText(stmt, 5)
            << " | Chan doan: ";

        if (sqlite3_column_type(
                stmt,
                6
            ) == SQLITE_NULL)
        {
            std::cout << "Chua nhap";
        }
        else
        {
            std::cout << getText(stmt, 6);
        }

        std::cout << '\n';
    }

    if (count == 0)
    {
        std::cout
            << "Khong co benh nhan nao "
            << "dang kham.\n";
    }

    sqlite3_finalize(stmt);
}

bool DangKhamManager::layPhienDangKham(
    int checkinId,
    int& patientId
)
{
    sqlite3* db = database.get();

    const char* sql = R"(
        SELECT patient_id
        FROM dang_kham
        WHERE checkin_id = ?
          AND end_time IS NULL;
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

    sqlite3_bind_int(
        stmt,
        1,
        checkinId
    );

    const bool found =
        sqlite3_step(stmt) == SQLITE_ROW;

    if (found)
    {
        patientId =
            sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return found;
}

void DangKhamManager::nhapChanDoan()
{
    sqlite3* db = database.get();

    std::cout << "Nhap check-in ID: ";

    int checkinId = 0;

    if (!(std::cin >> checkinId) ||
        checkinId <= 0)
    {
        std::cin.clear();

        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );

        std::cout
            << "Check-in ID khong hop le.\n";

        return;
    }

    int patientId = 0;

    if (!layPhienDangKham(
            checkinId,
            patientId
        ))
    {
        std::cout
            << "Benh nhan khong o "
            << "trang thai dang kham.\n";

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
        std::cout
            << "Chan doan khong duoc de trong.\n";

        return;
    }

    const char* sql = R"(
        UPDATE dang_kham
        SET
            chan_doan = ?,
            don_thuoc = ?,
            updated_at =
                datetime('now', 'localtime')
        WHERE checkin_id = ?
          AND end_time IS NULL;
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
        std::cerr
            << "Loi tao lenh cap nhat chan doan: "
            << sqlite3_errmsg(db)
            << '\n';

        return;
    }

    sqlite3_bind_text(
        stmt,
        1,
        chanDoan.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        stmt,
        2,
        donThuoc.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(
        stmt,
        3,
        checkinId
    );

    if (sqlite3_step(stmt) == SQLITE_DONE &&
        sqlite3_changes(db) > 0)
    {
        std::cout
            << "Da luu chan doan va don thuoc.\n";
    }
    else
    {
        std::cerr
            << "Khong cap nhat duoc chan doan.\n";
    }

    sqlite3_finalize(stmt);
}

void DangKhamManager::ketThucKham()
{
    sqlite3* db = database.get();

    std::cout
        << "Nhap check-in ID ket thuc kham: ";

    int checkinId = 0;

    if (!(std::cin >> checkinId) ||
        checkinId <= 0)
    {
        std::cin.clear();

        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );

        std::cout
            << "Check-in ID khong hop le.\n";

        return;
    }

    const char* sql = R"(
        UPDATE dang_kham
        SET
            end_time =
                datetime('now', 'localtime'),
            updated_at =
                datetime('now', 'localtime')
        WHERE checkin_id = ?
          AND end_time IS NULL;
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
        std::cerr
            << "Loi tao lenh ket thuc kham: "
            << sqlite3_errmsg(db)
            << '\n';

        return;
    }

    sqlite3_bind_int(
        stmt,
        1,
        checkinId
    );

    if (sqlite3_step(stmt) == SQLITE_DONE &&
        sqlite3_changes(db) > 0)
    {
        std::cout
            << "Da ket thuc phien kham.\n";
    }
    else
    {
        std::cout
            << "Khong tim thay phien "
            << "dang kham phu hop.\n";
    }

    sqlite3_finalize(stmt);
}

void DangKhamManager::hienThiLichSu()
{
    sqlite3* db = database.get();

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
        ORDER BY
            datetime(start_time) DESC,
            checkin_id DESC;
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
        std::cerr
            << "Loi doc lich su: "
            << sqlite3_errmsg(db)
            << '\n';

        return;
    }

    std::cout
        << "\n========== LICH SU KHAM ==========\n";

    int count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ++count;

        std::cout
            << "Check-in: "
            << sqlite3_column_int(stmt, 0)
            << " | BN: "
            << sqlite3_column_int(stmt, 1)
            << " | "
            << getText(stmt, 2)
            << "\nBac si: "
            << getText(stmt, 3)
            << " - "
            << getText(stmt, 4)
            << "\nBat dau: "
            << getText(stmt, 5)
            << "\nKet thuc: ";

        if (sqlite3_column_type(
                stmt,
                6
            ) == SQLITE_NULL)
        {
            std::cout << "Dang kham";
        }
        else
        {
            std::cout << getText(stmt, 6);
        }

        std::cout << "\nChan doan: ";

        if (sqlite3_column_type(
                stmt,
                7
            ) == SQLITE_NULL)
        {
            std::cout << "Chua co";
        }
        else
        {
            std::cout << getText(stmt, 7);
        }

        std::cout << "\nDon thuoc: ";

        if (sqlite3_column_type(
                stmt,
                8
            ) == SQLITE_NULL)
        {
            std::cout << "Chua co";
        }
        else
        {
            std::cout << getText(stmt, 8);
        }

        std::cout
            << "\n----------------------------------------\n";
    }

    if (count == 0)
        std::cout << "Chua co lich su kham.\n";

    sqlite3_finalize(stmt);
}

void DangKhamManager::chayMenu()
{
    while (true)
    {
        std::cout
            << "\n========== DANG KHAM ==========\n"
            << "1. Xem hang doi benh nhan\n"
            << "2. Bat dau kham\n"
            << "3. Xem benh nhan dang kham\n"
            << "4. Nhap chan doan va don thuoc\n"
            << "5. Ket thuc kham\n"
            << "6. Xem lich su kham\n"
            << "0. Thoat\n"
            << "Lua chon: ";

        int choice = -1;

        if (!(std::cin >> choice))
        {
            if (std::cin.eof() ||
                std::cin.bad())
            {
                break;
            }

            std::cin.clear();

            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );

            std::cout
                << "Lua chon khong hop le.\n";

            continue;
        }

        if (choice == 0)
            break;

        if (choice == 1)
            hienThiHangDoi();
        else if (choice == 2)
            batDauKham();
        else if (choice == 3)
            hienThiDangKham();
        else if (choice == 4)
            nhapChanDoan();
        else if (choice == 5)
            ketThucKham();
        else if (choice == 6)
            hienThiLichSu();
        else
            std::cout
                << "Lua chon khong hop le.\n";
    }
}
