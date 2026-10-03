#include "QuanLyKhamBenh.h"
#include "ThoiGian.h"
#include <iostream>
#include <limits>
#include <string>

int main() {
    const std::string DATABASE_PATH =
        "../TRUY_XUAT_BENH_NHAN/db/truyXuat.db";

    const std::string CSV_PATH =
        "db/bac_si_500_chia_khoa.csv";

    const std::string danhSachKhoa[] = {
        "Khoa Cap cuu",
        "Khoa Noi",
        "Khoa Ngoai",
        "Khoa Tim mach",
        "Khoa Nhi",
        "Khoa San",
        "Khoa Tai Mui Hong",
        "Khoa Mat",
        "Khoa Da lieu",
        "Khoa Than kinh"
    };

    QuanLyKhamBenh quanLy;

    if (!quanLy.KhoiDong(DATABASE_PATH, CSV_PATH)) {
        std::cerr
            << "\nKhoi dong that bai.\n"
            << "Hay kiem tra:\n"
            << "1. File truyXuat.db co ton tai.\n"
            << "2. Duong dan CSV bac si dung.\n"
            << "3. SQLite3 da duoc lien ket.\n";
        return 1;
    }

    while (true) {
        std::cout << "\n\n========================================\n";
        std::cout << "     HE THONG XU LY KHAM BENH\n";
        std::cout << "========================================\n";
        std::cout << "Thoi gian hien tai: "
                  << ThoiGian::DinhDang(ThoiGian::HienTai())
                  << "\n\n";

        for (int i = 0; i < 10; ++i) {
            std::cout << (i + 1) << ". " << danhSachKhoa[i] << "\n";
        }
        std::cout << "11. Xu ly tat ca cac khoa\n";
        std::cout << "12. Xem danh sach bac si cua mot khoa\n";
        std::cout << "13. Xem ket qua phien chay\n";
        std::cout << "0. Thoat\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Nhap lua chon: ";

        int luaChon;
        std::cin >> luaChon;

        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );
            std::cout << "Lua chon khong hop le.\n";
            continue;
        }

        if (luaChon == 0) {
            std::cout << "Da thoat chuong trinh.\n";
            break;
        }

        if (luaChon >= 1 && luaChon <= 10) {
            quanLy.XuLyKhoa(danhSachKhoa[luaChon - 1]);
        }
        else if (luaChon == 11) {
            quanLy.XuLyTatCaKhoa();
        }
        else if (luaChon == 12) {
            std::cout << "\nNhap so khoa (1-10): ";
            int soKhoa;
            std::cin >> soKhoa;

            if (soKhoa >= 1 && soKhoa <= 10) {
                quanLy.InDanhSachBacSiTheoKhoa(
                    danhSachKhoa[soKhoa - 1]
                );
            } else {
                std::cout << "So khoa khong hop le.\n";
            }
        }
        else if (luaChon == 13) {
            quanLy.HienThiKetQua();
        }
        else {
            std::cout << "Lua chon khong hop le.\n";
        }
    }

    return 0;
}
