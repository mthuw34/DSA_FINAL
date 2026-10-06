#include <iostream>
#include <string>
#include <sqlite3.h>
#include "../patient_validation.h"
#include "../HospitalPersistence.h"
#include "../../../include/SqliteMemoryTable.h"

using namespace std;

// Xóa thông tin bệnh nhân theo ID trong hospital.db.
int main() {
    sqlite3* db = nullptr;
    if (sqlite3_open("QUAN_LY_BENH_NHAN/db/hospital.db", &db) != SQLITE_OK) {
        cerr << "Khong mo duoc database\n";
        sqlite3_close(db);
        return 1;
    }
    if (sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        cerr << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return 1;
    }
    cout << "Nhap ID benh nhan can xoa: ";
    string input;
    getline(cin, input);
    int id;
    try {
        id = parseNonNegativeInt(input);
        if (id == 0) throw invalid_argument("ID phai lon hon 0");
    } catch (const exception&) {
        cerr << "ID khong hop le\n";
        sqlite3_close(db);
        return 1;
    }
    vector<Patient> patients;
    vector<CheckInRecord> checkIns;
    if (!HospitalPersistence::loadPatients(db, patients) ||
        !HospitalPersistence::loadCheckIns(db, checkIns)) { sqlite3_close(db); return 1; }
    if (!PatientCore::findPatient(patients, id)) {
        cout << "Khong tim thay benh nhan co ID = " << id << '\n';
        sqlite3_close(db); return 0;
    }
    if (PatientCore::latestCheckIn(checkIns, id)) {
        cerr << "Xoa that bai: benh nhan dang check-in.\n";
        sqlite3_close(db); return 1;
    }
    // Ghi thao tác xóa đã được quyết định ở tầng xử lý trong bộ nhớ.
    const bool removed = MemoryTable::erase(db, "patients", "id", id);
    if (!removed) cerr << "Xoa that bai: " << sqlite3_errmsg(db) << '\n';
    else cout << "Da xoa benh nhan co ID = " << id << '\n';
    sqlite3_close(db);
    return removed ? 0 : 1;
}
