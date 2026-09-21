#include <iostream>
#include <string>
#include <limits>
#include <sqlite3.h>

using namespace std;

int main() {
    sqlite3* db = nullptr;

    if (sqlite3_open("hospital.db", &db) != SQLITE_OK) {
        cout << "Khong mo duoc database: "
             << sqlite3_errmsg(db) << endl;
        return 1;
    }

    int id;

    cout << "Nhap ID benh nhan can sua: ";
    cin >> id;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    // =============================
    // 1. Tìm bệnh nhân
    // =============================

    const char* selectSql =
        "SELECT id, name, age, phone, birth_date, "
        "gender, hometown, address "
        "FROM patients WHERE id = ?;";

    sqlite3_stmt* selectStmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            selectSql,
            -1,
            &selectStmt,
            nullptr
        ) != SQLITE_OK) {

        cout << "Loi SQL: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_close(db);
        return 1;
    }

    sqlite3_bind_int(selectStmt, 1, id);

    if (sqlite3_step(selectStmt) != SQLITE_ROW) {
        cout << "Khong tim thay benh nhan co ID = "
             << id << endl;

        sqlite3_finalize(selectStmt);
        sqlite3_close(db);
        return 0;
    }

    auto getText = [&](int column) -> string {
        const unsigned char* text =
            sqlite3_column_text(selectStmt, column);

        if (text == nullptr)
            return "";

        return reinterpret_cast<const char*>(text);
    };

    string oldName = getText(1);
    int oldAge = sqlite3_column_int(selectStmt, 2);
    string oldPhone = getText(3);
    string oldBirthDate = getText(4);
    string oldGender = getText(5);
    string oldHometown = getText(6);
    string oldAddress = getText(7);

    cout << "\n===== THONG TIN HIEN TAI =====\n";

    cout << "ID: " << id << endl;
    cout << "Ten: " << oldName << endl;
    cout << "Tuoi: " << oldAge << endl;
    cout << "SDT: " << oldPhone << endl;
    cout << "Ngay sinh: " << oldBirthDate << endl;
    cout << "Gioi tinh: " << oldGender << endl;
    cout << "Que quan: " << oldHometown << endl;
    cout << "Dia chi: " << oldAddress << endl;

    sqlite3_finalize(selectStmt);

    // =============================
    // 2. Nhập thông tin mới
    // =============================

    cout << "\n===== NHAP THONG TIN MOI =====\n";
    cout << "Bo trong neu khong muon thay doi.\n\n";

    string input;

    cout << "Ten moi [" << oldName << "]: ";
    getline(cin, input);

    if (!input.empty())
        oldName = input;


    cout << "Tuoi moi [" << oldAge << "]: ";
    getline(cin, input);

    if (!input.empty()) {
        try {
            oldAge = stoi(input);
        }
        catch (...) {
            cout << "Tuoi khong hop le." << endl;
            sqlite3_close(db);
            return 1;
        }
    }


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

    // =============================
    // 3. UPDATE database
    // =============================

    const char* updateSql =
        "UPDATE patients "
        "SET name = ?, "
        "age = ?, "
        "phone = ?, "
        "birth_date = ?, "
        "gender = ?, "
        "hometown = ?, "
        "address = ? "
        "WHERE id = ?;";

    sqlite3_stmt* updateStmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            updateSql,
            -1,
            &updateStmt,
            nullptr
        ) != SQLITE_OK) {

        cout << "Loi SQL: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_close(db);
        return 1;
    }

    sqlite3_bind_text(
        updateStmt, 1,
        oldName.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(
        updateStmt,
        2,
        oldAge
    );

    sqlite3_bind_text(
        updateStmt, 3,
        oldPhone.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        updateStmt, 4,
        oldBirthDate.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        updateStmt, 5,
        oldGender.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        updateStmt, 6,
        oldHometown.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        updateStmt, 7,
        oldAddress.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(
        updateStmt,
        8,
        id
    );

    if (sqlite3_step(updateStmt) == SQLITE_DONE) {

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

    sqlite3_finalize(updateStmt);
    sqlite3_close(db);

    return 0;
}