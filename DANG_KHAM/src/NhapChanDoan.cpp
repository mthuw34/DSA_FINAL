#include <iostream>
#include <string>

void NhapChanDoan(
    const std::string& idBenhNhan,
    std::string& chanDoan,
    std::string& donThuoc
)
{
    std::cout << "\nBenh nhan ID: " << idBenhNhan << '\n';

    std::cout << "Chan doan cua benh nhan: ";
    std::getline(std::cin >> std::ws, chanDoan);

    std::cout << "Don thuoc cua benh nhan: ";
    std::getline(std::cin, donThuoc);
}
