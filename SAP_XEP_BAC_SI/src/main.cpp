#include "QuanLyKhamBenh.h"
#include "ThoiGian.h"
#include <iostream>
#include <limits>
#include <string>
using namespace std;

int main() {
    const string DATABASE_PATH =
        "../TRUY_XUAT_BENH_NHAN/db/truyXuat.db";

    const string CSV_PATH =
        "db/bac_si_500_chia_khoa.csv";

    const string danhSachKhoa[] = {
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
        cerr
            << "\nKhoi dong that bai.\n"
            << "Hay kiem tra:\n"
            << "1. File truyXuat.db co ton tai.\n"
            << "2. Duong dan CSV bac si dung.\n"
            << "3. SQLite3 da duoc lien ket.\n";
        return 1;
    }

    while (true) {
        cout << "\n\n========================================\n";
        cout << "    HE THONG XU LY KHAM BENH\n";
        cout << "========================================\n";
        cout << "Thoi gian hien tai: "
             << ThoiGian::DinhDang(ThoiGian::HienTai())
             << "\n\n";

        for (int i = 0; i < 10; ++i) {
            cout << (i + 1) << ". " << danhSachKhoa[i] << "\n";
        }
        cout << "11. Xu ly tat ca cac khoa\n";
        cout << "12. Xem danh sach bac si cua mot khoa\n";
        cout << "13. Xem ket qua phien chay\n";
        cout << "0. Thoat\n";
        cout << "----------------------------------------\n";
        cout << "Nhap lua chon: ";

        int luaChon;
        cin >> luaChon;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(
                numeric_limits<streamsize>::max(),
                '\n'
            );
            cout << "Lua chon khong hop le.\n";
            continue;
        }

        if (luaChon == 0) {
            cout << "Da thoat chuong trinh.\n";
            break;
        }

        if (luaChon >= 1 && luaChon <= 10) {
            quanLy.XuLyKhoa(danhSachKhoa[luaChon - 1]);
        }
        else if (luaChon == 11) {
            quanLy.XuLyTatCaKhoa();
        }
        else if (luaChon == 12) {
            cout << "\nNhap so khoa (1-10): ";
            int soKhoa;
            cin >> soKhoa;

            if (soKhoa >= 1 && soKhoa <= 10) {
                quanLy.InDanhSachBacSiTheoKhoa(
                    danhSachKhoa[soKhoa - 1]
                );
            } else {
                cout << "So khoa khong hop le.\n";
            }
        }
        else if (luaChon == 13) {
            quanLy.HienThiKetQua();
        }
        else {
            cout << "Lua chon khong hop le.\n";
        }
    }

    return 0;
}