#include <iostream>
#include <iomanip>
#include <string>
#include <sqlite3.h>

#include "patient_lookup.h"
#include "../patient_validation.h"
#include "../HospitalPersistence.h"

using namespace std;


// _________________________________________
// LAY TEXT TU SQLITE
// _________________________________________
// Nạp bảng bệnh nhân và tìm một ID bằng Linear Search trong bộ nhớ.
bool timBenhNhan(sqlite3* db, int patientId, Patient& patient)
{
    std::vector<Patient> records;
    if (!HospitalPersistence::loadPatients(db, records)) return false;
    const auto* selected = PatientCore::findPatient(records, patientId);
    if (!selected) return false;
    patient = *selected;
    try { patient.age = ageFromBirthDate(patient.birthDate); }
    catch (const std::exception& error) {
        cerr << "Ngay sinh khong hop le: " << error.what() << '\n';
        return false;
    }
    return true;
}

// ________________________________________
// HIEN THI THONG TIN BENH NHAN
// ________________________________________
// Hiển thị đầy đủ thông tin của một bệnh nhân.
void hienThiBenhNhan(
    const Patient& patient
)
{
    cout << "\n";
    cout << "________________________________________\n";
    cout << "          THONG TIN BENH NHAN\n";
    cout << "________________________________________\n";

    cout << "ID         : "
         << patient.id
         << '\n';

    cout << "Ho ten     : "
         << patient.name
         << '\n';

    cout << "Ngay sinh  : "
         << patient.birthDate
         << '\n';

    cout << "Tuoi       : "
         << patient.age
         << '\n';

    cout << "Gioi tinh  : "
         << patient.gender
         << '\n';

    cout << "Que quan   : "
         << patient.hometown
         << '\n';

    cout << "Dia chi    : "
         << patient.address
         << '\n';

    cout << "So DT      : "
         << patient.phone
         << '\n';


    // _________________________________________
    // CHIEU CAO
    // _________________________________________
    if (patient.height > 0)
    {
        cout << "Chieu cao  : "
             << fixed
             << setprecision(1)
             << patient.height
             << " cm\n";
    }
    else
    {
        cout << "Chieu cao  : Chua co du lieu\n";
    }


    // _________________________________________
    // CAN NANG
    // _________________________________________
    if (patient.weight > 0)
    {
        cout << "Can nang   : "
             << fixed
             << setprecision(1)
             << patient.weight
             << " kg\n";
    }
    else
    {
        cout << "Can nang   : Chua co du lieu\n";
    }


    // _________________________________________
    // BMI
    // _________________________________________
    if (patient.bmi > 0)
    {
        cout << "BMI        : "
             << fixed
             << setprecision(2)
             << patient.bmi
             << '\n';
    }
    else
    {
        cout << "BMI        : Chua co du lieu\n";
    }


    cout << "________________________________________\n";
}
