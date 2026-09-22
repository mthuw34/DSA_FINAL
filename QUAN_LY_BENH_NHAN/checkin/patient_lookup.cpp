#include <iostream>
#include <iomanip>
#include <string>
#include <sqlite3.h>

#include "patient_lookup.h"

using namespace std;


// =========================================
// LAY TEXT TU SQLITE
// =========================================
static string getText(
    sqlite3_stmt* stmt,
    int column
)
{
    const unsigned char* text =
        sqlite3_column_text(
            stmt,
            column
        );

    if (text == nullptr)
        return "";

    return reinterpret_cast<const char*>(text);
}


// =========================================
// TIM BENH NHAN THEO ID
// =========================================
bool timBenhNhan(
    sqlite3* db,
    int patientId,
    Patient& patient
)
{
    const char* sql = R"(

        SELECT
            id,
            name,
            age,
            phone,
            birth_date,
            gender,
            hometown,
            address,
            height,
            weight,
            bmi

        FROM patients

        WHERE id = ?;

    )";

    sqlite3_stmt* stmt = nullptr;


    // CHUAN BI SQL
    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        cerr
            << "Loi SQL tim benh nhan: "
            << sqlite3_errmsg(db)
            << '\n';

        return false;
    }


    // GAN ID VAO ?
    sqlite3_bind_int(
        stmt,
        1,
        patientId
    );


    // KHONG TIM THAY
    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);

        return false;
    }


    // =========================================
    // LAY DU LIEU
    // =========================================

    patient.id =
        sqlite3_column_int(
            stmt,
            0
        );

    patient.name =
        getText(
            stmt,
            1
        );

    patient.age =
        sqlite3_column_int(
            stmt,
            2
        );

    patient.phone =
        getText(
            stmt,
            3
        );

    patient.birthDate =
        getText(
            stmt,
            4
        );

    patient.gender =
        getText(
            stmt,
            5
        );

    patient.hometown =
        getText(
            stmt,
            6
        );

    patient.address =
        getText(
            stmt,
            7
        );


    // =========================================
    // CHIEU CAO
    // =========================================
    if (sqlite3_column_type(stmt, 8) == SQLITE_NULL)
    {
        patient.height = 0;
    }
    else
    {
        patient.height =
            sqlite3_column_double(
                stmt,
                8
            );
    }


    // =========================================
    // CAN NANG
    // =========================================
    if (sqlite3_column_type(stmt, 9) == SQLITE_NULL)
    {
        patient.weight = 0;
    }
    else
    {
        patient.weight =
            sqlite3_column_double(
                stmt,
                9
            );
    }


    // =========================================
    // BMI
    // =========================================
    if (sqlite3_column_type(stmt, 10) == SQLITE_NULL)
    {
        patient.bmi = 0;
    }
    else
    {
        patient.bmi =
            sqlite3_column_double(
                stmt,
                10
            );
    }


    sqlite3_finalize(stmt);

    return true;
}


// =========================================
// HIEN THI THONG TIN BENH NHAN
// =========================================
void hienThiBenhNhan(
    const Patient& patient
)
{
    cout << "\n";
    cout << "========================================\n";
    cout << "          THONG TIN BENH NHAN\n";
    cout << "========================================\n";

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


    // =========================================
    // CHIEU CAO
    // =========================================
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


    // =========================================
    // CAN NANG
    // =========================================
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


    // =========================================
    // BMI
    // =========================================
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


    cout << "========================================\n";
}