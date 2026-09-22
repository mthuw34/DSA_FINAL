#include <iostream>
#include <limits>
#include <vector>
#include <filesystem>
#include <sqlite3.h>

#include "check_in.h"
#include "../bang_check_in/bang_check_in.h"

using namespace std;

static string resolveDatabasePath(const char* argv0)
{
    vector<string> candidates;

    if (argv0 != nullptr && argv0[0] != '\0')
    {
        filesystem::path exePath(argv0);

        if (exePath.is_relative())
        {
            exePath = filesystem::absolute(exePath);
        }

        if (!exePath.empty())
        {
            candidates.push_back((exePath.parent_path() / "hospital.db").string());
            candidates.push_back((exePath.parent_path().parent_path() / "hospital.db").string());
            candidates.push_back((exePath.parent_path().parent_path() / "QUAN_LY_BENH_NHAN" / "hospital.db").string());
        }
    }

    candidates.push_back((filesystem::current_path() / "hospital.db").string());
    candidates.push_back("hospital.db");

    for (const string& candidate : candidates)
    {
        if (filesystem::exists(candidate))
        {
            return candidate;
        }
    }

    return "hospital.db";
}

int main(int argc, char* argv[])
{
    sqlite3* db = nullptr;

    const string databasePath = resolveDatabasePath(argc > 0 ? argv[0] : nullptr);

    // =========================================
    // MO DATABASE
    // =========================================
    if (sqlite3_open(
            databasePath.c_str(),
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
        cout << "3. Xoa toan bo du lieu check-in\n";
        cout << "4. Xoa mot check-in\n";
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
        // 1. CHECK-IN
        // =====================================
        if (choice == 1)
        {
            checkInMotBenhNhan(db);
        }


        // =====================================
        // 2. XEM BANG CHECK-IN
        // =====================================
        else if (choice == 2)
        {
            hienThiBangCheckIn(db);
        }


        // =====================================
        // 3. XOA TOAN BO CHECK-IN
        // =====================================
        else if (choice == 3)
        {
            char xacNhan;

            cout << "\n";
            cout << "========================================\n";
            cout << "            CANH BAO\n";
            cout << "========================================\n";

            cout << "Ban co chac muon xoa TOAN BO check-in?\n";
            cout << "Du lieu sau khi xoa se khong con trong bang.\n";
            cout << "Xac nhan (y/n): ";

            cin >> xacNhan;

            if (xacNhan == 'y' || xacNhan == 'Y')
            {
                xoaToanBoCheckIn(db);
            }
            else
            {
                cout << "Da huy thao tac xoa.\n";
            }
        }


        // =====================================
        // 4. XOA MOT CHECK-IN
        // =====================================
        else if (choice == 4)
        {
            int checkinId;

            cout << "\n";
            cout << "========================================\n";
            cout << "          XOA MOT CHECK-IN\n";
            cout << "========================================\n";

            cout << "Nhap ma check-in can xoa: ";

            if (!(cin >> checkinId))
            {
                cout << "Ma check-in khong hop le.\n";

                cin.clear();

                cin.ignore(
                    numeric_limits<streamsize>::max(),
                    '\n'
                );

                continue;
            }


            if (checkinId <= 0)
            {
                cout << "Ma check-in phai lon hon 0.\n";
                continue;
            }


            char xacNhan;

            cout << "Ban co chac muon xoa check-in co ma "
                 << checkinId
                 << "? (y/n): ";

            cin >> xacNhan;


            if (xacNhan == 'y' || xacNhan == 'Y')
            {
                xoaMotCheckIn(
                    db,
                    checkinId
                );
            }
            else
            {
                cout << "Da huy thao tac xoa.\n";
            }
        }


        // =====================================
        // 0. THOAT
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


    // =========================================
    // DONG DATABASE
    // =========================================
    sqlite3_close(db);

    cout << "\nDa thoat chuong trinh.\n";

    return 0;
}