#include "DangKhamManager.h"

#include <iostream>
#include <limits>

#include "NhapChanDoan.h"

using namespace std;

// Lấy dữ liệu TEXT từ kết quả truy vấn SQLite.
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

// Khởi động module DANG_KHAM và đồng bộ dữ liệu bác sĩ đã được xếp.
bool DangKhamManager::khoiDong(
    const string& sourcePath,
    const string& destinationPath
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

    if (!database.dongBoTuXepBacSi())
        return false;

    return true;
}

// Hiển thị danh sách bệnh nhân đang trong quá trình khám.
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
            chan_doan,
            don_thuoc,
            loi_nhac_bac_si
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
        cerr
            << "Loi doc dang_kham: "
            << sqlite3_errmsg(db)
            << '\n';

        return;
    }

    cout
        << "\n========== BENH NHAN DANG KHAM ==========\n";

    int count = 0;

    int result = SQLITE_ROW;
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        ++count;

        cout
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
            << "\n   Chan doan: "
            << (sqlite3_column_type(stmt, 6) == SQLITE_NULL
                    ? "Chua nhap"
                    : getText(stmt, 6))
            << "\n   Don thuoc: "
            << (sqlite3_column_type(stmt, 7) == SQLITE_NULL
                    ? "Chua nhap"
                    : getText(stmt, 7))
            << "\n   Loi nhac bac si: "
            << (sqlite3_column_type(stmt, 8) == SQLITE_NULL
                    ? "Chua nhap"
                    : getText(stmt, 8))
            << '\n';
    }

    if (result != SQLITE_DONE)
        cerr << "Loi doc danh sach dang kham: " << sqlite3_errmsg(db) << '\n';
    else if (count == 0)
    {
        cout
            << "Khong co benh nhan nao dang kham.\n";
    }

    sqlite3_finalize(stmt);
}

// Kiểm tra và lấy bệnh nhân theo check-in ID nếu vẫn đang khám.
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
        patientId = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return found;
}

// Nhập và lưu chẩn đoán, đơn thuốc và lời nhắc của bác sĩ.
void DangKhamManager::nhapChanDoan()
{
    sqlite3* db = database.get();

    cout << "Nhap check-in ID: ";

    int checkinId = 0;

    if (!(cin >> checkinId) ||
        checkinId <= 0)
    {
        cin.clear();

        cin.ignore(
            numeric_limits<streamsize>::max(),
            '\n'
        );

        cout
            << "Check-in ID khong hop le.\n";

        return;
    }

    int patientId = 0;

    if (!layPhienDangKham(
            checkinId,
            patientId
        ))
    {
        cout
            << "Benh nhan khong o trang thai dang kham.\n";

        return;
    }

    string chanDoan;
    string donThuoc;
    string loiNhacBacSi;

    NhapChanDoan(
        to_string(patientId),
        chanDoan,
        donThuoc,
        loiNhacBacSi
    );

    if (!cin || chanDoan.find_first_not_of(" \t\r\n") == string::npos)
    {
        cout
            << "Chan doan khong duoc de trong.\n";

        return;
    }

    const char* sql = R"(
        UPDATE dang_kham
        SET
            chan_doan = ?,
            don_thuoc = ?,
            loi_nhac_bac_si = ?,
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
        cerr
            << "Loi tao lenh cap nhat: "
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

    sqlite3_bind_text(
        stmt,
        3,
        loiNhacBacSi.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(
        stmt,
        4,
        checkinId
    );

    if (sqlite3_step(stmt) == SQLITE_DONE &&
        sqlite3_changes(db) > 0)
    {
        cout
            << "Da luu chan doan, don thuoc va loi nhac bac si.\n";
    }
    else
    {
        cerr
            << "Khong cap nhat duoc thong tin kham.\n";
    }

    sqlite3_finalize(stmt);
}

// Kết thúc phiên khám và ghi thời gian kết thúc.
void DangKhamManager::ketThucKham()
{
    sqlite3* db = database.get();

    cout
        << "Nhap check-in ID ket thuc kham: ";

    int checkinId = 0;

    if (!(cin >> checkinId) ||
        checkinId <= 0)
    {
        cin.clear();

        cin.ignore(
            numeric_limits<streamsize>::max(),
            '\n'
        );

        cout
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
        cerr
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
        cout
            << "Da ket thuc phien kham.\n";
    }
    else
    {
        cout
            << "Khong tim thay phien dang kham phu hop.\n";
    }

    sqlite3_finalize(stmt);
}

// Hiển thị và xử lý menu chính của module DANG_KHAM.
void DangKhamManager::chayMenu()
{
    while (true)
    {
        cout
            << "\n========== DANG KHAM ==========\n"
            << "1. Xem benh nhan dang kham\n"
            << "2. Nhap chan doan, don thuoc va loi nhac bac si\n"
            << "3. Ket thuc kham\n"
            << "0. Thoat\n"
            << "Lua chon: ";

        int choice = -1;

        if (!(cin >> choice))
        {
            if (cin.eof() ||
                cin.bad())
            {
                break;
            }

            cin.clear();

            cin.ignore(
                numeric_limits<streamsize>::max(),
                '\n'
            );

            cout
                << "Lua chon khong hop le.\n";

            continue;
        }

        if (choice == 0)
            break;

        // Nhận ca vừa bắt đầu khi chương trình đang mở.
        if (choice >= 1 && choice <= 3 && !database.dongBoTuXepBacSi())
            continue;

        if (choice == 1)
            hienThiDangKham();
        else if (choice == 2)
            nhapChanDoan();
        else if (choice == 3)
            ketThucKham();
        else
            cout
                << "Lua chon khong hop le.\n";
    }
}
