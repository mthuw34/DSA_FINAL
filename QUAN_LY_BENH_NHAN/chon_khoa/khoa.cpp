#include <iostream>
#include <string>
#include <ctime>

#include "khoa.h"

std::string chonKhoa()
{
    int luaChon;

    while (true)
    {
        std::cout << "\n";
        std::cout << "============================\n";
        std::cout << "          CHON KHOA\n";
        std::cout << "============================\n";

        std::cout << "1. Khoa Cap cuu\n";
        std::cout << "2. Khoa Noi\n";
        std::cout << "3. Khoa Ngoai\n";
        std::cout << "4. Khoa Tim mach\n";
        std::cout << "5. Khoa Nhi\n";
        std::cout << "6. Khoa San\n";
        std::cout << "7. Khoa Tai Mui Hong\n";
        std::cout << "8. Khoa Mat\n";
        std::cout << "9. Khoa Da lieu\n";
        std::cout << "10. Khoa Than kinh\n";

        std::cout << "\nNhap khoa (1-10): ";
        std::cin >> luaChon;

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
                std::cout << "Lua chon khong hop le!\n";
        }
    }
}

bool khoaDangHoatDong(
    const std::string& department,
    int priority,
    std::string& lyDo
)
{
    // ===============================
    // LAY GIO THUC TE CUA MAY
    // ===============================
    std::time_t now = std::time(nullptr);

    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    int hour = localTime.tm_hour;
    int minute = localTime.tm_min;

    int hienTai = hour * 60 + minute;

    // In ra de debug
    std::cout << "\nGio thuc te hien tai: ";

    if (hour < 10)
        std::cout << "0";

    std::cout << hour << ":";

    if (minute < 10)
        std::cout << "0";

    std::cout << minute << '\n';


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

    const int SANG_KET_THUC = 11 * 60 + 30;
    // 11:30

    const int CHIEU_BAT_DAU = 13 * 60;
    // 13:00

    const int CHIEU_KET_THUC = 16 * 60 + 30;
    // 16:30


    // ===============================
    // CA SANG
    // ===============================
    if (
        hienTai >= SANG_BAT_DAU &&
        hienTai < SANG_KET_THUC
    )
    {
        lyDo =
            "Khoa dang trong gio lam viec buoi sang.";

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
                "Khoa dang nghi trua. "
                "Benh nhan uu tien cao nen chuyen sang Khoa Cap cuu.";
        }
        else
        {
            lyDo =
                "Khoa dang nghi trua 11:30 - 13:00. "
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
            "Khoa dang trong gio lam viec buoi chieu.";

        return true;
    }


    // ===============================
    // NGOAI GIO
    // ===============================
    if (priority <= 2)
    {
        lyDo =
            "Khoa da het gio lam viec. "
            "Benh nhan uu tien cao nen chuyen sang Khoa Cap cuu.";
    }
    else
    {
        lyDo =
            "Khoa da het gio lam viec. "
            "Khong nhan check-in.";
    }

    return false;
}