#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sqlite3.h>
#include "../patient_validation.h"
#include "../HospitalPersistence.h"

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
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);";

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

    // Khóa ghi khi nạp dữ liệu và kiểm tra trùng bằng bảng băm trong bộ nhớ.
    if (sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        cerr << sqlite3_errmsg(db) << '\n';
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }
    vector<Patient> existing;
    PatientCore::HashIndex identities;
    if (!HospitalPersistence::loadPatients(db, existing)) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sqlite3_finalize(stmt); sqlite3_close(db); return 1;
    }
    for (size_t i = 0; i < existing.size(); ++i)
        identities.put(PatientCore::importKey(existing[i]), i);

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

            Patient candidate;
            candidate.name = name; candidate.birthDate = birthDate;
            candidate.gender = gender; candidate.hometown = hometown;
            candidate.address = address; candidate.phone = phone;
            const string identity = PatientCore::importKey(candidate);
            size_t position;
            if (identities.find(identity, position)) { ++skipped; continue; }

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
                identities.put(identity, static_cast<size_t>(success));
                ++success;
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
