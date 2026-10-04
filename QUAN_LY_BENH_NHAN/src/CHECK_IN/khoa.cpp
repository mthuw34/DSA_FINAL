#include <iostream>
#include <string>
#include <ctime>
#include <limits>

#include "khoa.h"

using namespace std;

string chonKhoa()
{
    int luaChon;

    while (true)
    {
        cout << "\n";
        cout << "============================\n";
        cout << "          CHON KHOA\n";
        cout << "============================\n";

        cout << "1. Khoa Cap cuu\n";
        cout << "2. Khoa Noi\n";
        cout << "3. Khoa Ngoai\n";
        cout << "4. Khoa Tim mach\n";
        cout << "5. Khoa Nhi\n";
        cout << "6. Khoa San\n";
        cout << "7. Khoa Tai Mui Hong\n";
        cout << "8. Khoa Mat\n";
        cout << "9. Khoa Da lieu\n";
        cout << "10. Khoa Than kinh\n";

        cout << "\nNhap khoa (1-10): ";
        if (!(cin >> luaChon))
        {
            if (cin.eof() || cin.bad())
                return "";

            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Lua chon khong hop le!\n";
            continue;
        }

        switch (luaChon)
        {
            case 1: return "Khoa Cap cuu";
            case 2: return "Khoa Noi";
            case 3: return "Khoa Ngoai";
            case 4: return "Khoa Tim mach";
            case 5: return "Khoa Nhi";
            case 6: return "Khoa San";
            case 7: return "Khoa Tai Mui Hong";
            case 8: return "Khoa Mat";
            case 9: return "Khoa Da lieu";
            case 10: return "Khoa Than kinh";

            default:
                cout << "Lua chon khong hop le!\n";
        }
    }
}

bool khoaDangHoatDong(
    const string& department,
    int priority,
    string& lyDo
)
{
    // ===============================
    // LAY GIO THUC TE CUA MAY
    // ===============================
    time_t now = time(nullptr);

    tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    int hour = localTime.tm_hour;
    int minute = localTime.tm_min;

    int hienTai = hour * 60 + minute;

    // In ra de debug
    cout << "\nGio thuc te hien tai: ";

    if (hour < 10)
        cout << "0";

    cout << hour << ":";

    if (minute < 10)
        cout << "0";

    cout << minute << '\n';


    // ===============================
    // KHOA CAP CUU 24/7
    // ===============================
    if (department == "Khoa Cap cuu")
    {
        lyDo = "Khoa Cap cuu hoat dong 24/7.";

        return true;
    }


    // ===============================
    // GIO LAM VIEC KHOA THUONG
    // ===============================

    const int SANG_BAT_DAU = 7 * 60 + 30;
    // 07:30

    const int SANG_KET_THUC = 10 * 60;
    // 10:00

    const int CHIEU_BAT_DAU = 13 * 60;
    // 13:00

    const int CHIEU_KET_THUC = 15 * 60;
    // 15:00


    // ===============================
    // CA SANG
    // ===============================
    if (
        hienTai >= SANG_BAT_DAU &&
        hienTai < SANG_KET_THUC
    )
    {
        lyDo =
            "Khoa dang nhan check-in buoi sang 07:30 - 10:00.";

        return true;
    }


    // ===============================
    // NGHI TRUA
    // ===============================
    if (
        hienTai >= SANG_KET_THUC &&
        hienTai < CHIEU_BAT_DAU
    )
    {
        if (priority <= 2)
        {
            lyDo =
                "Ngoai gio nhan check-in (07:30 - 10:00 va 13:00 - 15:00). "
                "Benh nhan uu tien cao nen chuyen sang Khoa Cap cuu.";
        }
        else
        {
            lyDo =
                "Ngoai gio nhan check-in (07:30 - 10:00 va 13:00 - 15:00). "
                "Khong nhan check-in.";
        }

        return false;
    }


    // ===============================
    // CA CHIEU
    // ===============================
    if (
        hienTai >= CHIEU_BAT_DAU &&
        hienTai < CHIEU_KET_THUC
    )
    {
        lyDo =
            "Khoa dang nhan check-in buoi chieu 13:00 - 15:00.";

        return true;
    }


    // ===============================
    // NGOAI GIO
    // ===============================
    if (priority <= 2)
    {
        lyDo =
            "Ngoai gio nhan check-in (07:30 - 10:00 va 13:00 - 15:00). "
            "Benh nhan uu tien cao nen chuyen sang Khoa Cap cuu.";
    }
    else
    {
        lyDo =
            "Ngoai gio nhan check-in (07:30 - 10:00 va 13:00 - 15:00). "
            "Khong nhan check-in.";
    }

    return false;
}
