#include <iostream>
#include <string>
#include <limits>
#include <sqlite3.h>
#include "../patient_validation.h"
#include "../HospitalPersistence.h"
#include "../../../include/SqliteMemoryTable.h"

using namespace std;

// Tìm và cập nhật thông tin bệnh nhân theo ID.
int main() {
    sqlite3* db = nullptr;

    if (sqlite3_open("QUAN_LY_BENH_NHAN/db/hospital.db", &db) != SQLITE_OK) {
        cout << "Khong mo duoc database: "
             << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return 1;
    }

    int id;

    cout << "Nhap ID benh nhan can sua: ";
    string idInput;
    getline(cin, idInput);
    try {
        id = parseNonNegativeInt(idInput);
        if (id <= 0) throw invalid_argument("ID khong hop le");
    } catch (const exception&) {
        cerr << "ID khong hop le\n";
        sqlite3_close(db);
        return 1;
    }


    // _____________________________________
    // 1. Tìm bệnh nhân
    // _____________________________________

    vector<Patient> patients;
    if (!HospitalPersistence::loadPatients(db, patients)) {
        sqlite3_close(db); return 1;
    }
    const auto* found = PatientCore::findPatient(patients, id);
    if (!found) {
        cout << "Khong tim thay benh nhan co ID = " << id << '\n';
        sqlite3_close(db); return 0;
    }
    const Patient& selected = *found;
    string oldName = selected.name;
    int oldAge = selected.age;
    string oldPhone = selected.phone, oldBirthDate = selected.birthDate;
    string oldGender = selected.gender, oldHometown = selected.hometown, oldAddress = selected.address;

    cout << "\n===== THONG TIN HIEN TAI =====\n";

    cout << "ID: " << id << endl;
    cout << "Ten: " << oldName << endl;
    try {
        cout << "Tuoi: " << ageFromBirthDate(oldBirthDate) << endl;
    } catch (const exception&) {
        cout << "Ngay sinh cu khong hop le; vui long sua ngay sinh." << endl;
    }
    cout << "SDT: " << oldPhone << endl;
    cout << "Ngay sinh: " << oldBirthDate << endl;
    cout << "Gioi tinh: " << oldGender << endl;
    cout << "Que quan: " << oldHometown << endl;
    cout << "Dia chi: " << oldAddress << endl;


    // _____________________________________
    // 2. Nhập thông tin mới
    // _____________________________________

    cout << "\n===== NHAP THONG TIN MOI =====\n";
    cout << "Bo trong neu khong muon thay doi.\n\n";

    string input;

    cout << "Ten moi [" << oldName << "]: ";
    getline(cin, input);

    if (!input.empty())
        oldName = input;


    // Tuoi duoc tinh tu ngay sinh, khong nhap rieng.
    cout << "SDT moi [" << oldPhone << "]: ";
    getline(cin, input);

    if (!input.empty())
        oldPhone = input;


    cout << "Ngay sinh moi [" << oldBirthDate
         << "] (YYYY-MM-DD): ";

    getline(cin, input);

    if (!input.empty())
        oldBirthDate = input;


    cout << "Gioi tinh moi [" << oldGender << "]: ";
    getline(cin, input);

    if (!input.empty())
        oldGender = input;


    cout << "Que quan moi [" << oldHometown << "]: ";
    getline(cin, input);

    if (!input.empty())
        oldHometown = input;


    cout << "Dia chi moi [" << oldAddress << "]: ";
    getline(cin, input);

    if (!input.empty())
        oldAddress = input;

    // _____________________________________
    // 3. UPDATE database
    // _____________________________________

    if (!cin) {
        cerr << "Nhap thong tin chua hoan tat; da huy cap nhat.\n";
        sqlite3_close(db);
        return 1;
    }

    try {
        oldAge = ageFromBirthDate(oldBirthDate);
    } catch (const std::exception& error) {
        cerr << error.what() << '\n';
        sqlite3_close(db);
        return 1;
    }

    bool updated = false;
    {
        MemoryTable::Transaction transaction(db);
        MemoryTable::Table table;
        if (transaction && table.load(db, "patients")) {
            auto* row = table.find("id", id); // Linear Search cho mot ID trong snapshot moi.
            if (row) {
                updated = MemoryTable::set(db, table, *row, "name", oldName) &&
                    MemoryTable::set(db, table, *row, "age", to_string(oldAge)) &&
                    MemoryTable::set(db, table, *row, "phone", oldPhone) &&
                    MemoryTable::set(db, table, *row, "birth_date", oldBirthDate) &&
                    MemoryTable::set(db, table, *row, "gender", oldGender) &&
                    MemoryTable::set(db, table, *row, "hometown", oldHometown) &&
                    MemoryTable::set(db, table, *row, "address", oldAddress) &&
                    MemoryTable::insert(db, "patients", table, *row, "id") &&
                    transaction.commit();
            }
        }
    }
    if (updated) {

        cout << "\nCap nhat thong tin thanh cong!"
             << endl;

        cout << "Benh nhan ID = "
             << id
             << " da duoc cap nhat."
             << endl;

    } else {

        cout << "Cap nhat that bai: "
             << sqlite3_errmsg(db)
             << endl;
    }

    sqlite3_close(db);

    return updated ? 0 : 1;
}
