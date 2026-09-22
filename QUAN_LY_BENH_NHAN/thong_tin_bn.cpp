#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sqlite3.h>

// Doc 1 dong CSV, co xu ly truong hop dia chi co dau phay
std::vector<std::string> parseCSV(const std::string& line)
{
    std::vector<std::string> fields;
    std::string field;
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

    fields.push_back(field);

    return fields;
}

int main()
{
    sqlite3* db = nullptr;

    if (sqlite3_open("QUAN_LY_BENH_NHAN/hospital.db", &db) != SQLITE_OK)
    {
        std::cerr << "Loi mo database: "
                  << sqlite3_errmsg(db) << '\n';

        sqlite3_close(db);
        return 1;
    }

    std::ifstream file("benh_nhan_20000.csv");

    if (!file.is_open())
    {
        std::cerr << "Khong mo duoc patients.csv\n";
        sqlite3_close(db);
        return 1;
    }

    const char* sql =
        "INSERT INTO patients "
        "(name, birth_date, age, gender, hometown, address, phone) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        std::cerr << "Loi SQL: "
                  << sqlite3_errmsg(db) << '\n';

        file.close();
        sqlite3_close(db);
        return 1;
    }

    // Import 20.000 dong nhanh hon rat nhieu
    sqlite3_exec(
        db,
        "BEGIN TRANSACTION;",
        nullptr,
        nullptr,
        nullptr
    );

    std::string line;

    // Bo qua dong tieu de Excel
    std::getline(file, line);

    int success = 0;
    int failed = 0;
    int row = 1;

    while (std::getline(file, line))
    {
        ++row;

        if (line.empty())
            continue;

        // Xoa \r cuoi dong tren Windows
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        std::vector<std::string> data = parseCSV(line);

        if (data.size() != 7)
        {
            std::cerr
                << "Dong " << row
                << " khong du 7 cot\n";

            ++failed;
            continue;
        }

        try
        {
            std::string name      = data[0];
            std::string birthDate = data[1];
            int age               = std::stoi(data[2]);
            std::string gender    = data[3];
            std::string hometown  = data[4];
            std::string address   = data[5];
            std::string phone     = data[6];

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
                ++success;
            }
            else
            {
                std::cerr
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
            std::cerr
                << "Du lieu sai tai dong "
                << row << '\n';

            ++failed;

            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        }
    }

    sqlite3_exec(
        db,
        "COMMIT;",
        nullptr,
        nullptr,
        nullptr
    );

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    file.close();

    std::cout << "\n===== IMPORT HOAN TAT =====\n";
    std::cout << "Thanh cong: " << success << '\n';
    std::cout << "That bai:    " << failed << '\n';

    return 0;
}