#include <iostream>
#include <string>
#include <limits>
#include <sqlite3.h>

#include "check_in.h"
#include "patient_lookup.h"

#include "../chon_khoa/khoa.h"

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


static string getPriorityName(int priority)
{
    switch (priority)
    {
        case 1:
            return "Cap cuu";

        case 2:
            return "Rat cao";

        case 3:
            return "Cao";

        case 4:
            return "Binh thuong";

        case 5:
            return "Thap";

        default:
            return "Khong xac dinh";
    }
}


bool checkInMotBenhNhan(sqlite3* db)
{
    // =====================================
    // NHAP ID
    // =====================================

    int patientId;

    cout << "\n\n";
    cout << "========================================\n";
    cout << "          CHECK-IN BENH VIEN\n";
    cout << "========================================\n";
    cout << "Nhap ID benh nhan\n";
    cout << "Nhap 0 de thoat\n";
    cout << "----------------------------------------\n";
    cout << "ID: ";

    if (!(cin >> patientId))
    {
        cout << "ID khong hop le!\n";

        cin.clear();

        cin.ignore(
            numeric_limits<streamsize>::max(),
            '\n'
        );

        return true;
    }

    if (patientId == 0)
        return false;


    // =====================================
    // TIM BENH NHAN
    // =====================================

    Patient patient;

    if (!timBenhNhan(
            db,
            patientId,
            patient
        ))
    {
        cout << "Khong tim thay benh nhan co ID = "
             << patientId
             << '\n';

        return true;
    }

    hienThiBenhNhan(patient);


    // =====================================
    // CHON KHOA
    // =====================================

    string department =
        chonKhoa();

    cout << "\nDa chon: "
         << department
         << '\n';


    // =====================================
    // CHON UU TIEN
    // =====================================

    int priority;

    cout << "\n";
    cout << "========================================\n";
    cout << "             MUC DO UU TIEN\n";
    cout << "========================================\n";

    cout << "1. Cap cuu\n";
    cout << "2. Rat cao\n";
    cout << "3. Cao\n";
    cout << "4. Binh thuong\n";
    cout << "5. Thap\n";

    cout << "Nhap muc do (1-5): ";

    if (!(cin >> priority))
    {
        cout << "Du lieu khong hop le!\n";

        cin.clear();

        cin.ignore(
            numeric_limits<streamsize>::max(),
            '\n'
        );

        return true;
    }

    if (priority < 1 || priority > 5)
    {
        cout << "Muc do uu tien phai tu 1 den 5.\n";
        return true;
    }


    // =====================================
    // KIEM TRA GIO HOAT DONG
    // =====================================

    string lyDo;

    bool duocNhan =
        khoaDangHoatDong(
            department,
            priority,
            lyDo
        );

    cout << "\nTrang thai khoa: "
         << lyDo
         << '\n';


    if (!duocNhan)
    {
        // Uu tien 1 hoac 2
        if (priority <= 2)
        {
            char choice;

            cout << "\nBenh nhan co muc uu tien cao.\n";

            cout
                << "Chuyen sang Khoa Cap cuu? (y/n): ";

            cin >> choice;

            if (
                choice == 'y' ||
                choice == 'Y'
            )
            {
                department =
                    "Khoa Cap cuu";

                cout
                    << "Da chuyen sang Khoa Cap cuu.\n";
            }
            else
            {
                cout << "Da huy check-in.\n";
                return true;
            }
        }

        // Ca thuong
        else
        {
            cout << "\n";
            cout << "========================================\n";
            cout << "          KHONG THE CHECK-IN\n";
            cout << "========================================\n";

            cout << lyDo << '\n';

            cout << "========================================\n";

            return true;
        }
    }


    // =====================================
    // INSERT CHECK-IN
    // =====================================

    const char* insertSql = R"(

        INSERT INTO checkins
        (
            patient_id,
            department,
            priority
        )

        VALUES (?, ?, ?);

    )";

    sqlite3_stmt* insertStmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            insertSql,
            -1,
            &insertStmt,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Loi SQL check-in: "
             << sqlite3_errmsg(db)
             << '\n';

        return true;
    }

    sqlite3_bind_int(
        insertStmt,
        1,
        patientId
    );

    sqlite3_bind_text(
        insertStmt,
        2,
        department.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(
        insertStmt,
        3,
        priority
    );

    if (sqlite3_step(insertStmt) != SQLITE_DONE)
    {
        cerr << "Check-in that bai: "
             << sqlite3_errmsg(db)
             << '\n';

        sqlite3_finalize(insertStmt);

        return true;
    }

    sqlite3_finalize(insertStmt);


    sqlite3_int64 checkinId =
        sqlite3_last_insert_rowid(db);


    // =====================================
    // LAY THOI GIAN CHECK-IN TU DATABASE
    // =====================================

    const char* showSql = R"(

        SELECT
            checkin_time,
            department,
            priority

        FROM checkins

        WHERE checkin_id = ?;

    )";

    sqlite3_stmt* showStmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            showSql,
            -1,
            &showStmt,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Loi doc phieu check-in: "
             << sqlite3_errmsg(db)
             << '\n';

        return true;
    }

    sqlite3_bind_int64(
        showStmt,
        1,
        checkinId
    );


    // =====================================
    // IN PHIEU
    // =====================================

    if (sqlite3_step(showStmt) == SQLITE_ROW)
    {
        string checkinTime =
            getText(showStmt, 0);

        string savedDepartment =
            getText(showStmt, 1);

        int savedPriority =
            sqlite3_column_int(showStmt, 2);


        cout << "\n\n";
        cout << "========================================\n";
        cout << "             PHIEU CHECK-IN\n";
        cout << "========================================\n";

        cout << "Ma check-in : "
             << checkinId
             << '\n';

        cout << "ID benh nhan: "
             << patient.id
             << '\n';

        cout << "Ho ten      : "
             << patient.name
             << '\n';

        cout << "Ngay sinh   : "
             << patient.birthDate
             << '\n';

        cout << "Tuoi        : "
             << patient.age
             << '\n';

        cout << "Gioi tinh   : "
             << patient.gender
             << '\n';

        cout << "Que quan    : "
             << patient.hometown
             << '\n';

        cout << "Dia chi     : "
             << patient.address
             << '\n';

        cout << "So DT       : "
             << patient.phone
             << '\n';

        cout << "Check-in    : "
             << checkinTime
             << '\n';

        cout << "Khoa        : "
             << savedDepartment
             << '\n';

        cout << "Uu tien     : "
             << savedPriority
             << " - "
             << getPriorityName(savedPriority)
             << '\n';

        cout << "========================================\n";
    }

    sqlite3_finalize(showStmt);

    cout << "\nCheck-in hoan tat.";
    cout << "\nSan sang cho benh nhan tiep theo.\n";

    return true;
}