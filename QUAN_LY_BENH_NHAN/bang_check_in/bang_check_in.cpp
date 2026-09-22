#include <iostream>
#include <iomanip>
#include <string>
#include <sqlite3.h>

#include "bang_check_in.h"

using namespace std;

static string getText(
    sqlite3_stmt* stmt,
    int column
)
{
    const unsigned char* text =
        sqlite3_column_text(stmt, column);

    if (text == nullptr)
        return "";

    return reinterpret_cast<const char*>(text);
}

static string tenUuTien(int priority)
{
    switch (priority)
    {
        case 1: return "Cap cuu";
        case 2: return "Rat cao";
        case 3: return "Cao";
        case 4: return "Binh thuong";
        case 5: return "Thap";

        default:
            return "Khong xac dinh";
    }
}

void hienThiBangCheckIn(sqlite3* db)
{
    const char* sql = R"(

        SELECT
            c.department,
            c.checkin_id,
            p.id,
            p.name,
            c.priority,
            c.checkin_time

        FROM checkins c

        JOIN patients p
            ON c.patient_id = p.id

        ORDER BY

            CASE c.department

                WHEN 'Khoa Cap cuu' THEN 1
                WHEN 'Khoa Noi' THEN 2
                WHEN 'Khoa Ngoai' THEN 3
                WHEN 'Khoa Tim mach' THEN 4
                WHEN 'Khoa Nhi' THEN 5
                WHEN 'Khoa San' THEN 6
                WHEN 'Khoa Tai Mui Hong' THEN 7
                WHEN 'Khoa Mat' THEN 8
                WHEN 'Khoa Da lieu' THEN 9
                WHEN 'Khoa Than kinh' THEN 10

                ELSE 99

            END ASC,

            c.priority ASC,
            c.checkin_time ASC,
            p.id ASC,
            c.checkin_id ASC;

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
        cerr << "Loi doc bang check-in: "
             << sqlite3_errmsg(db)
             << '\n';

        return;
    }

    string khoaHienTai = "";

    int stt = 0;
    int tongBenhNhan = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        string department =
            getText(stmt, 0);

        int checkinId =
            sqlite3_column_int(stmt, 1);

        int patientId =
            sqlite3_column_int(stmt, 2);

        string name =
            getText(stmt, 3);

        int priority =
            sqlite3_column_int(stmt, 4);

        string checkinTime =
            getText(stmt, 5);

        // Khoa moi
        if (department != khoaHienTai)
        {
            khoaHienTai = department;
            stt = 0;

            cout << "\n\n";
            cout << "====================================================================\n";
            cout << "                         " << department << '\n';
            cout << "====================================================================\n";

            cout
                << left
                << setw(6)  << "STT"
                << setw(10) << "ID"
                << setw(25) << "Ho ten"
                << setw(20) << "Uu tien"
                << setw(22) << "Check-in"
                << '\n';

            cout << "--------------------------------------------------------------------\n";
        }

        stt++;
        tongBenhNhan++;

        cout
            << left
            << setw(6) << stt
            << setw(10) << patientId
            << setw(25) << name.substr(0, 23)
            << setw(20)
            << (
                to_string(priority)
                + " - "
                + tenUuTien(priority)
            )
            << setw(22) << checkinTime
            << '\n';
    }

    sqlite3_finalize(stmt);

    if (tongBenhNhan == 0)
    {
        cout << "\nChua co benh nhan nao check-in.\n";
        return;
    }

    cout << "\n====================================================================\n";
    cout << "Tong so check-in: "
         << tongBenhNhan
         << '\n';
    cout << "====================================================================\n";
}

bool xoaToanBoCheckIn(sqlite3* db)
{
    const char* sql =
        "DELETE FROM checkins;";

    char* errorMessage = nullptr;

    int result = sqlite3_exec(
        db,
        sql,
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK)
    {
        std::cerr
            << "Loi khi xoa du lieu check-in: "
            << errorMessage
            << '\n';

        sqlite3_free(errorMessage);

        return false;
    }

    std::cout
        << "\nDa xoa toan bo du lieu trong bang checkins.\n";

    return true;
}
bool xoaMotCheckIn(
    sqlite3* db,
    int checkinId
)
{
    const char* sql =
        "DELETE FROM checkins WHERE checkin_id = ?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Loi SQL khi xoa check-in: "
             << sqlite3_errmsg(db)
             << '\n';

        return false;
    }

    sqlite3_bind_int(
        stmt,
        1,
        checkinId
    );

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        cerr << "Khong the xoa check-in: "
             << sqlite3_errmsg(db)
             << '\n';

        sqlite3_finalize(stmt);

        return false;
    }

    int soDongDaXoa =
        sqlite3_changes(db);

    sqlite3_finalize(stmt);

    if (soDongDaXoa == 0)
    {
        cout << "\nKhong tim thay check-in co ma = "
             << checkinId
             << '\n';

        return false;
    }

    cout << "\nDa xoa check-in co ma = "
         << checkinId
         << '\n';

    return true;
}