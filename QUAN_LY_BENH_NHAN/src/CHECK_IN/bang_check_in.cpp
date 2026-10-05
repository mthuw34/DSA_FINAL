#include <iostream>
#include <iomanip>
#include <string>
#include <sqlite3.h>

#include "bang_check_in.h"
#include "../HospitalPersistence.h"

using namespace std;

// Chuyển mã mức độ ưu tiên thành tên để hiển thị.
static string tenUuTien(int priority)
{
    switch (priority)
    {
        case 1: return "Cap cuu";
        case 2: return "Rat cao";
        case 3: return "Cao";
        case 4: return "Binh thuong";
        case 5: return "Thap";
        default: return "Khong xac dinh";
    }
}

// Hiển thị danh sách bệnh nhân đã check-in theo khoa và mức ưu tiên.
void hienThiBangCheckIn(sqlite3* db)
{
    vector<Patient> patients;
    vector<CheckInRecord> checkIns;
    if (!HospitalPersistence::loadPatients(db, patients) ||
        !HospitalPersistence::loadCheckIns(db, checkIns)) return;
    const auto patientIds = PatientCore::indexPatients(patients);
    PatientCore::sortCheckIns(checkIns);

    string khoaHienTai;
    int stt = 0;
    int tongBenhNhan = 0;

    for (const auto& record : checkIns)
    {
        size_t position;
        if (!patientIds.find(to_string(record.patientId), position)) continue;
        const string& department = record.department;
        int checkinId = record.id, patientId = record.patientId, priority = record.priority;
        const string& name = patients[position].name;
        const string& checkinTime = record.time;

        if (department != khoaHienTai)
        {
            khoaHienTai = department;
            stt = 0;

            cout << "\n\n";
            cout << "_____________________________________________________________\n";
            cout << "                         " << department << '\n';
            cout << "_____________________________________________________________\n";

            cout
                << left
                << setw(6)  << "STT"
                << setw(12) << "Ma check-in"
                << setw(10) << "ID"
                << setw(25) << "Ho ten"
                << setw(20) << "Uu tien"
                << setw(22) << "Check-in"
                << '\n';

            cout << "_____________________________________________________________\n";
        }

        ++stt;
        ++tongBenhNhan;

        cout
            << left
            << setw(6) << stt
            << setw(12) << checkinId
            << setw(10) << patientId
            << setw(25) << name.substr(0, 23)
            << setw(20) << (to_string(priority) + " - " + tenUuTien(priority))
            << setw(22) << checkinTime
            << '\n';
    }

    if (tongBenhNhan == 0)
    {
        cout << "\nChua co benh nhan nao check-in.\n";
        return;
    }

    cout << "\n_____________________________________________________________\n";
    cout << "Tong so check-in: " << tongBenhNhan << '\n';
    cout << "_____________________________________________________________\n";
}

// Xóa toàn bộ check-in, giữ bộ đếm để mã mới không trùng với lịch sử khám.
bool xoaToanBoCheckIn(sqlite3* db)
{
    char* errorMessage = nullptr;

    const char* sql = R"(
        BEGIN IMMEDIATE;

        DELETE FROM checkins;

        COMMIT;
    )";

    const int result = sqlite3_exec(
        db,
        sql,
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK)
    {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);

        cerr << "Loi khi xoa du lieu check-in: "
             << (errorMessage ? errorMessage : sqlite3_errmsg(db))
             << '\n';

        sqlite3_free(errorMessage);
        return false;
    }

    cout << "\nDa xoa toan bo du lieu trong bang checkins.\n";
    cout << "Ma check-in moi se tiep tuc tang de khong trung lich su kham.\n";

    return true;
}

// Xóa một check-in theo mã check-in.
bool xoaMotCheckIn(sqlite3* db, int checkinId)
{
    if (checkinId <= 0)
    {
        cout << "\nMa check-in phai lon hon 0.\n";
        return false;
    }

    vector<CheckInRecord> checkIns;
    if (!HospitalPersistence::loadCheckIns(db, checkIns)) return false;
    if (!PatientCore::findCheckIn(checkIns, checkinId)) {
        cout << "Khong tim thay check-in co ma = " << checkinId << '\n';
        return false;
    }
    // Chỉ lưu thao tác xóa bản ghi đã tìm được trong bộ nhớ.
    const char* sql =
        "DELETE FROM checkins WHERE checkin_id = ?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        cerr << "Loi SQL khi xoa check-in: "
             << sqlite3_errmsg(db)
             << '\n';

        return false;
    }

    sqlite3_bind_int(stmt, 1, checkinId);

    const int stepResult = sqlite3_step(stmt);

    if (stepResult != SQLITE_DONE)
    {
        cerr << "Khong the xoa check-in: "
             << sqlite3_errmsg(db)
             << '\n';

        sqlite3_finalize(stmt);
        return false;
    }

    const int soDongDaXoa = sqlite3_changes(db);
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
