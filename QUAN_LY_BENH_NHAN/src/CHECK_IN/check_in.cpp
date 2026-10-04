#include <iostream>
#include <string>
#include <limits>
#include <sqlite3.h>

#include "check_in.h"
#include "patient_lookup.h"
#include "khoa.h"
#include "../HospitalPersistence.h"

using namespace std;


// Đổi mức ưu tiên từ số sang tên hiển thị.
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


// Thực hiện toàn bộ quy trình check-in cho một bệnh nhân.
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
        if (cin.eof() || cin.bad())
            return false;

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
    // KIEM TRA BENH NHAN DA CHECK-IN CHUA
    // =====================================

    vector<CheckInRecord> checkIns;
    if (!HospitalPersistence::loadCheckIns(db, checkIns)) return true;
    const auto* existing = PatientCore::latestCheckIn(checkIns, patientId);
    if (existing)
    {
        cout << "\nBENH NHAN DA CHECK-IN\n"
             << "Ma check-in : " << existing->id << '\n'
             << "ID benh nhan: " << patientId << '\n'
             << "Ho ten      : " << patient.name << '\n'
             << "Khoa        : " << existing->department << '\n'
             << "Thoi gian   : " << existing->time << '\n'
             << "Uu tien     : " << existing->priority << " - "
             << getPriorityName(existing->priority) << '\n'
             << "Khong the check-in lan thu hai!\n";
        return true;
    }
    // =====================================
    // CHON KHOA
    // =====================================

    string department =
        chonKhoa();

    if (department.empty())
        return false;

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
        if (cin.eof() || cin.bad())
            return false;

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

            if (!(cin >> choice))
                return false;

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

    // Khóa ghi ngắn, nạp lại dữ liệu rồi kiểm tra trùng bằng C++ để hai phiên không nhận trùng.
    if (sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        cerr << sqlite3_errmsg(db) << '\n'; return true;
    }
    if (!HospitalPersistence::loadCheckIns(db, checkIns) ||
        PatientCore::latestCheckIn(checkIns, patientId)) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        cerr << "Khong the check-in; du lieu da thay doi hoac benh nhan da check-in.\n";
        return true;
    }

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
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
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
        if (sqlite3_extended_errcode(db) == SQLITE_CONSTRAINT_UNIQUE)
            cerr << "Benh nhan da duoc check-in o phien khac.\n";
        else
            cerr << "Check-in that bai: " << sqlite3_errmsg(db) << '\n';

        sqlite3_finalize(insertStmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);

        return true;
    }

    sqlite3_finalize(insertStmt);


    sqlite3_int64 checkinId =
        sqlite3_last_insert_rowid(db);


    if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        cerr << sqlite3_errmsg(db) << '\n';
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return true;
    }
    // Nạp bảng rồi tìm phiếu mới trong bộ nhớ, không truy vấn chọn ID bằng SQL.
    if (!HospitalPersistence::loadCheckIns(db, checkIns)) return true;
    const auto* saved = PatientCore::findCheckIn(checkIns, static_cast<int>(checkinId));
    if (saved)
    {
        const string& checkinTime = saved->time;
        const string& savedDepartment = saved->department;
        const int savedPriority = saved->priority;

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


    cout << "\nCheck-in hoan tat.";
    cout << "\nSan sang cho benh nhan tiep theo.\n";

    return true;
}
