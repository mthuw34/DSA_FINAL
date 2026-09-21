#include <iostream>
#include <string>
#include <sqlite3.h>

#include "patient_lookup.h"

static std::string getText(
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
            birth_date,
            age,
            gender,
            hometown,
            address,
            phone

        FROM patients

        WHERE id = ?;

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
            << "Loi SQL tim benh nhan: "
            << sqlite3_errmsg(db)
            << '\n';

        return false;
    }

    sqlite3_bind_int(
        stmt,
        1,
        patientId
    );

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return false;
    }

    patient.id =
        sqlite3_column_int(stmt, 0);

    patient.name =
        getText(stmt, 1);

    patient.birthDate =
        getText(stmt, 2);

    patient.age =
        sqlite3_column_int(stmt, 3);

    patient.gender =
        getText(stmt, 4);

    patient.hometown =
        getText(stmt, 5);

    patient.address =
        getText(stmt, 6);

    patient.phone =
        getText(stmt, 7);

    sqlite3_finalize(stmt);

    return true;
}


void hienThiBenhNhan(
    const Patient& patient
)
{
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "          THONG TIN BENH NHAN\n";
    std::cout << "========================================\n";

    std::cout << "ID         : "
              << patient.id << '\n';

    std::cout << "Ho ten     : "
              << patient.name << '\n';

    std::cout << "Ngay sinh  : "
              << patient.birthDate << '\n';

    std::cout << "Tuoi       : "
              << patient.age << '\n';

    std::cout << "Gioi tinh  : "
              << patient.gender << '\n';

    std::cout << "Que quan   : "
              << patient.hometown << '\n';

    std::cout << "Dia chi    : "
              << patient.address << '\n';

    std::cout << "So DT      : "
              << patient.phone << '\n';

    std::cout << "========================================\n";
}