#include <array>
#include <filesystem>
#include <iostream>
#include <limits>
#include <sqlite3.h>

using namespace std;

static const array<const char*, 12> deleteStatements = {
    "DELETE FROM main.checkins WHERE checkin_id = ?;",
    "DELETE FROM priority.priority_checkins WHERE checkin_id = ?;",
    "DELETE FROM retrieval.queue_khoa_cap_cuu WHERE checkin_id = ?;",
    "DELETE FROM retrieval.queue_khoa_noi WHERE checkin_id = ?;",
    "DELETE FROM retrieval.queue_khoa_ngoai WHERE checkin_id = ?;",
    "DELETE FROM retrieval.queue_khoa_tim_mach WHERE checkin_id = ?;",
    "DELETE FROM retrieval.queue_khoa_nhi WHERE checkin_id = ?;",
    "DELETE FROM retrieval.queue_khoa_san WHERE checkin_id = ?;",
    "DELETE FROM retrieval.queue_khoa_tai_mui_hong WHERE checkin_id = ?;",
    "DELETE FROM retrieval.queue_khoa_mat WHERE checkin_id = ?;",
    "DELETE FROM retrieval.queue_khoa_da_lieu WHERE checkin_id = ?;",
    "DELETE FROM retrieval.queue_khoa_than_kinh WHERE checkin_id = ?;"
};

static bool executeSql(sqlite3* db, const char* sql)
{
    char* error = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &error) == SQLITE_OK)
        return true;

    cerr << "Loi database: " << (error ? error : sqlite3_errmsg(db)) << '\n';
    sqlite3_free(error);
    return false;
}

static bool executeDelete(sqlite3* db, const char* sql, int checkinId)
{
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(statement, 1, checkinId);
    const bool success = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return success;
}

static bool checkinExists(sqlite3* db, int checkinId)
{
    const char* sql = R"(
        SELECT EXISTS(SELECT 1 FROM main.checkins WHERE checkin_id = ?)
            OR EXISTS(SELECT 1 FROM priority.priority_checkins WHERE checkin_id = ?);
    )";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(statement, 1, checkinId);
    sqlite3_bind_int(statement, 2, checkinId);
    const bool exists = sqlite3_step(statement) == SQLITE_ROW
        && sqlite3_column_int(statement, 0) != 0;
    sqlite3_finalize(statement);
    return exists;
}

static bool deleteOneCheckin(sqlite3* db, int checkinId)
{
    if (!checkinExists(db, checkinId))
    {
        cout << "Khong tim thay check-in ID " << checkinId << ".\n";
        return false;
    }

    if (!executeSql(db, "BEGIN IMMEDIATE;"))
        return false;

    for (const char* sql : deleteStatements)
    {
        if (!executeDelete(db, sql, checkinId))
        {
            executeSql(db, "ROLLBACK;");
            return false;
        }
    }

    if (!executeSql(db, "COMMIT;"))
    {
        executeSql(db, "ROLLBACK;");
        return false;
    }

    cout << "Da xoa check-in ID " << checkinId << " khoi cac database.\n";
    return true;
}

static bool deleteAllCheckins(sqlite3* db)
{
    if (!executeSql(db, "BEGIN IMMEDIATE;"))
        return false;

    for (const char* sql : deleteStatements)
    {
        string deleteAllSql(sql);
        const size_t wherePosition = deleteAllSql.find(" WHERE checkin_id = ?");
        if (wherePosition != string::npos)
            deleteAllSql.erase(wherePosition);

        if (!executeSql(db, deleteAllSql.c_str()))
        {
            executeSql(db, "ROLLBACK;");
            return false;
        }
    }

    if (!executeSql(db, "COMMIT;"))
    {
        executeSql(db, "ROLLBACK;");
        return false;
    }

    cout << "Da xoa toan bo check-in khoi cac database.\n";
    return true;
}

int main()
{
    const array<const char*, 3> databasePaths = {
        "QUAN_LY_BENH_NHAN/db/hospital.db",
        "THAY_DOI_MUC_DO_UU_TIEN/db/priority.db",
        "TRUY_XUAT_BENH_NHAN/db/truyXuat.db"
    };

    for (const char* path : databasePaths)
    {
        if (!filesystem::exists(path))
        {
            cerr << "Khong tim thay database: " << path << '\n';
            cerr << "Hay chay chuong trinh tu thu muc goc workspace.\n";
            return 1;
        }
    }

    sqlite3* db = nullptr;
    if (sqlite3_open(databasePaths[0], &db) != SQLITE_OK)
    {
        cerr << "Khong mo duoc hospital.db: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return 1;
    }

    const string attachSql =
        "ATTACH DATABASE '" + string(databasePaths[1]) + "' AS priority;"
        "ATTACH DATABASE '" + string(databasePaths[2]) + "' AS retrieval;";
    if (!executeSql(db, attachSql.c_str()))
    {
        sqlite3_close(db);
        return 1;
    }

    while (true)
    {
        cout << "\n========== XOA CHECK-IN ==========\n";
        cout << "1. Xoa mot check-in\n";
        cout << "2. Xoa toan bo check-in\n";
        cout << "0. Thoat\n";
        cout << "Lua chon: ";

        int choice = -1;
        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Lua chon khong hop le.\n";
            continue;
        }

        if (choice == 0)
            break;

        if (choice == 1)
        {
            cout << "Nhap check-in ID can xoa: ";
            int checkinId = 0;
            if (!(cin >> checkinId) || checkinId <= 0)
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Check-in ID khong hop le.\n";
                continue;
            }
            deleteOneCheckin(db, checkinId);
        }
        else if (choice == 2)
        {
            cout << "Xac nhan xoa toan bo check-in? (y/n): ";
            char confirmation = 'n';
            if (!(cin >> confirmation))
                break;
            if (confirmation == 'y' || confirmation == 'Y')
                deleteAllCheckins(db);
            else
                cout << "Da huy thao tac.\n";
        }
        else
        {
            cout << "Lua chon khong hop le.\n";
        }
    }

    sqlite3_close(db);
    cout << "Da thoat chuong trinh.\n";
    return 0;
}