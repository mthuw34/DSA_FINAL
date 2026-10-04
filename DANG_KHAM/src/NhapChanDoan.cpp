#include "NhapChanDoan.h"

#include <iostream>

void NhapChanDoan(
    const std::string& idBenhNhan,
    std::string& chanDoan,
    std::string& donThuoc,
    std::string& loiNhacBacSi
)
{
    std::cout
        << "\nBenh nhan ID: "
        << idBenhNhan
        << '\n';

    std::cout
        << "Chan doan cua benh nhan: ";

    std::getline(
        std::cin >> std::ws,
        chanDoan
    );

    std::cout
        << "Don thuoc cua benh nhan: ";

    std::getline(
        std::cin,
        donThuoc
    );

    std::cout
        << "Loi nhac cua bac si: ";

    std::getline(
        std::cin,
        loiNhacBacSi
    );
}
