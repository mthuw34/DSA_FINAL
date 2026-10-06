#include <iostream>
#include <limits>
#include <vector>
#include <filesystem>
#include <sqlite3.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "check_in.h"
#include "bang_check_in.h"

using namespace std;

// Lấy đường dẫn file chương trình đang chạy.
static filesystem::path getExecutablePath(const char* argv0)
{
#ifdef _WIN32
    vector<wchar_t> buffer(512);

    while (true)
    {
        DWORD length = GetModuleFileNameW(
            nullptr,
            buffer.data(),
            static_cast<DWORD>(buffer.size())
        );

        if (length == 0)
            break;

        if (length < buffer.size() - 1)
            return filesystem::path(buffer.data(), buffer.data() + length);

        buffer.resize(buffer.size() * 2);
    }
#endif

    if (argv0 != nullptr && argv0[0] != '\0')
        return filesystem::path(argv0);

    return {};
}

// Tìm đường dẫn hospital.db phù hợp khi chạy chương trình.
static string resolveDatabasePath(const char* argv0)
{
    vector<filesystem::path> candidates;

    filesystem::path exePath = getExecutablePath(argv0);

    if (!exePath.empty())
    {
        if (exePath.is_relative())
        {
            exePath = filesystem::absolute(exePath);
        }

        candidates.push_back(exePath.parent_path().parent_path() / "db" / "hospital.db");
        candidates.push_back(exePath.parent_path().parent_path().parent_path() / "QUAN_LY_BENH_NHAN" / "db" / "hospital.db");
    }

    candidates.push_back(filesystem::current_path() / "QUAN_LY_BENH_NHAN" / "db" / "hospital.db");
    candidates.push_back(filesystem::current_path() / "db" / "hospital.db");

    for (const filesystem::path& candidate : candidates)
    {
        if (filesystem::exists(candidate))
        {
            return candidate.string();
        }
    }

    return "QUAN_LY_BENH_NHAN/db/hospital.db";
}

// Mở database và điều khiển menu quản lý check-in.
int main(int argc, char* argv[])
{
    sqlite3* db = nullptr;

    const string databasePath = resolveDatabasePath(argc > 0 ? argv[0] : nullptr);

    // _____________________________________
    // MO DATABASE
    // _____________________________________
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

    sqlite3_busy_timeout(db, 5000);

    if (sqlite3_exec(
            db,
            "PRAGMA foreign_keys = ON;",
            nullptr,
            nullptr,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Khong the bat foreign key: "
             << sqlite3_errmsg(db) << '\n';
        sqlite3_close(db);
        return 1;
    }

    // Ràng buộc toàn vẹn khi lưu; việc tìm và kiểm tra trùng được thực hiện bằng C++.
    if (sqlite3_exec(db,
            "CREATE UNIQUE INDEX IF NOT EXISTS ux_checkins_patient_id "
            "ON checkins(patient_id);",
            nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        cerr << "Khong the thiet lap rang buoc check-in: "
             << sqlite3_errmsg(db) << '\n';
        cerr << "Neu co patient_id trung, can xu ly cac check-in trung truoc.\n";
        sqlite3_close(db);
        return 1;
    }

    cout << "Da ket noi database.\n";


    // _____________________________________
    // MENU CHINH
    // _____________________________________
    while (true)
    {
        cout << "\n\n";
        cout << "_____________________________________\n";
        cout << "          QUAN LY CHECK-IN\n";
        cout << "_____________________________________\n";

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
            if (cin.eof() || cin.bad())
                break;

            cout << "Lua chon khong hop le.\n";

            cin.clear();

            cin.ignore(
                numeric_limits<streamsize>::max(),
                '\n'
            );

            continue;
        }


        // _____________________________________
        // 1. CHECK-IN
        // _____________________________________
        if (choice == 1)
        {
            checkInMotBenhNhan(db);
        }


        // _____________________________________
        // 2. XEM BANG CHECK-IN
        // _____________________________________
        else if (choice == 2)
        {
            hienThiBangCheckIn(db);
        }


        // _____________________________________
        // 3. XOA TOAN BO CHECK-IN
        // _____________________________________
        else if (choice == 3)
        {
            char xacNhan;

            cout << "\n";
            cout << "_____________________________________\n";
            cout << "            CANH BAO\n";
            cout << "_____________________________________\n";

            cout << "Ban co chac muon xoa TOAN BO check-in?\n";
            cout << "Du lieu sau khi xoa se khong con trong bang.\n";
            cout << "Xac nhan (y/n): ";

            if (!(cin >> xacNhan))
                break;

            if (xacNhan == 'y' || xacNhan == 'Y')
            {
                xoaToanBoCheckIn(db);
            }
            else
            {
                cout << "Da huy thao tac xoa.\n";
            }
        }


        // _____________________________________
        // 4. XOA MOT CHECK-IN
        // _____________________________________
        else if (choice == 4)
        {
            int checkinId;

            cout << "\n";
            cout << "_____________________________________\n";
            cout << "          XOA MOT CHECK-IN\n";
            cout << "_____________________________________\n";

            cout << "Nhap ma check-in can xoa: ";

            if (!(cin >> checkinId))
            {
                if (cin.eof() || cin.bad())
                    break;

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

            if (!(cin >> xacNhan))
                break;


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


        // _____________________________________
        // 0. THOAT
        // _____________________________________
        else if (choice == 0)
        {
            break;
        }


        // _____________________________________
        // NHAP SAI
        // _____________________________________
        else
        {
            cout << "Lua chon khong hop le.\n";
        }
    }


    // _____________________________________
    // DONG DATABASE
    // _____________________________________
    sqlite3_close(db);

    cout << "\nDa thoat chuong trinh.\n";

    return 0;
}
