#include <iostream>
#include <limits>
#include <sqlite3.h>

#include "check_in.h"
#include "../bang_check_in/bang_check_in.h"

using namespace std;

int main()
{
    sqlite3* db = nullptr;

    const char* databasePath = "hospital.db";

    const string dbPath = "hospital.db";
  

    // MO DATABASE
    if (sqlite3_open(
            databasePath,
            &db
        ) != SQLITE_OK)
    {
        cerr << "Khong mo duoc database!\n";
        cerr << sqlite3_errmsg(db) << '\n';

        sqlite3_close(db);

        return 1;
    }

    sqlite3_exec(
        db,
        "PRAGMA foreign_keys = ON;",
        nullptr,
        nullptr,
        nullptr
    );

    cout << "Da ket noi database.\n";


    // =========================================
    // MENU CHINH
    // =========================================
    while (true)
    {
        cout << "\n\n";
        cout << "========================================\n";
        cout << "          QUAN LY CHECK-IN\n";
        cout << "========================================\n";

        cout << "1. Check-in benh nhan\n";
        cout << "2. Xem bang check-in\n";
        cout << "0. Thoat\n";

        cout << "----------------------------------------\n";
        cout << "Lua chon: ";


        int choice;

        if (!(cin >> choice))
        {
            cout << "Lua chon khong hop le.\n";

            cin.clear();

            cin.ignore(
                numeric_limits<streamsize>::max(),
                '\n'
            );

            continue;
        }


        // =====================================
        // CHECK-IN
        // =====================================
        if (choice == 1)
        {
            checkInMotBenhNhan(db);
        }


        // =====================================
        // XEM BANG CHECK-IN
        // =====================================
        else if (choice == 2)
        {
            hienThiBangCheckIn(db);
        }


        // =====================================
        // THOAT
        // =====================================
        else if (choice == 0)
        {
            break;
        }


        // =====================================
        // NHAP SAI
        // =====================================
        else
        {
            cout << "Lua chon khong hop le.\n";
        }
    }


    sqlite3_close(db);

    cout << "\nDa thoat chuong trinh.\n";

    return 0;
}