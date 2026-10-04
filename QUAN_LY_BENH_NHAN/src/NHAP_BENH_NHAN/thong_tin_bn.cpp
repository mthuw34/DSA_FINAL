#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sqlite3.h>
#include "../patient_validation.h"

using namespace std;

// Doc 1 dong CSV, co xu ly truong hop dia chi co dau phay
// Tách một dòng CSV thành các trường dữ liệu, có xử lý dấu phẩy trong dấu ngoặc kép.
vector<string> parseCSV(const string& line)
{
    vector<string> fields;
    string field;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i)
    {
        char c = line[i];

        if (c == '"')
        {
            // "" ben trong chuoi -> dau "
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"')
            {
                field += '"';
                ++i;
            }
            else
            {
                inQuotes = !inQuotes;
            }
        }
        else if (c == ',' && !inQuotes)
        {
            fields.push_back(field);
            field.clear();
        }
        else
        {
            field += c;
        }
    }

    if (inQuotes) return {};
    fields.push_back(field);

    return fields;
}

// Đọc file CSV và thêm danh sách bệnh nhân vào hospital.db.
int main()
{
    sqlite3* db = nullptr;

    if (sqlite3_open("QUAN_LY_BENH_NHAN/db/hospital.db", &db) != SQLITE_OK)
    {
        cerr << "Loi mo database: "
                  << sqlite3_errmsg(db) << '\n';

        sqlite3_close(db);
        return 1;
    }

    ifstream file("QUAN_LY_BENH_NHAN/db/benh_nhan_20000.csv");

    if (!file.is_open())
    {
        cerr << "Khong mo duoc patients.csv\n";
        sqlite3_close(db);
        return 1;
    }

    const char* sql =
        "INSERT INTO patients "
        "(name, birth_date, age, gender, hometown, address, phone) "
        "SELECT ?1, ?2, ?3, ?4, ?5, ?6, ?7 WHERE NOT EXISTS ("
        "SELECT 1 FROM patients WHERE name = ?1 AND birth_date = ?2 "
        "AND gender IS ?4 AND hometown IS ?5 AND address IS ?6 AND phone IS ?7);";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Loi SQL: "
                  << sqlite3_errmsg(db) << '\n';

        file.close();
        sqlite3_close(db);
        return 1;
    }

    // Serialize imports so repeated/concurrent imports cannot duplicate records.
    if (sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        cerr << sqlite3_errmsg(db) << '\n';
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }
    if (sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS ix_patients_import ON patients(name, birth_date, phone);",
                     nullptr, nullptr, nullptr) != SQLITE_OK) {
        cerr << sqlite3_errmsg(db) << '\n';
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    string line;

    // Bo qua dong tieu de Excel
    getline(file, line);
    if (line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != "name,birth_date,age,gender,hometown,address,phone") {
        cerr << "Tieu de CSV khong hop le\n";
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    int success = 0;
    int failed = 0;
    int skipped = 0;
    int row = 1;

    while (getline(file, line))
    {
        ++row;

        if (line.empty())
            continue;

        // Xoa \r cuoi dong tren Windows
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        vector<string> data = parseCSV(line);

        if (data.size() != 7)
        {
            cerr
                << "Dong " << row
                << " khong du 7 cot\n";

            ++failed;
            continue;
        }

        try
        {
            string name      = data[0];
            string birthDate = data[1];
            int age               = ageFromBirthDate(birthDate);
            parseNonNegativeInt(data[2]);
            string gender    = data[3];
            string hometown  = data[4];
            string address   = data[5];
            string phone     = data[6];

            sqlite3_bind_text(
                stmt, 1,
                name.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                stmt, 2,
                birthDate.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_int(
                stmt, 3,
                age
            );

            sqlite3_bind_text(
                stmt, 4,
                gender.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                stmt, 5,
                hometown.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                stmt, 6,
                address.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                stmt, 7,
                phone.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            if (sqlite3_step(stmt) == SQLITE_DONE)
            {
                if (sqlite3_changes(db) > 0) ++success;
                else ++skipped;
            }
            else
            {
                cerr
                    << "Loi dong "
                    << row
                    << ": "
                    << sqlite3_errmsg(db)
                    << '\n';

                ++failed;
            }

            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        }
        catch (...)
        {
            cerr
                << "Du lieu sai tai dong "
                << row << '\n';

            ++failed;

            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        }
    }

    if (file.bad()) ++failed;
    const bool committed = failed == 0 &&
        sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr) == SQLITE_OK;
    if (!committed) {
        cerr << "Import that bai; da huy cac thay doi: " << sqlite3_errmsg(db) << '\n';
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        success = 0;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    file.close();

    cout << "\n===== IMPORT HOAN TAT =====\n";
    cout << "Thanh cong: " << success << '\n';
    cout << "That bai:    " << failed << '\n';

    cout << "Bo qua trung: " << skipped << '\n';
    return committed ? 0 : 1;
}
