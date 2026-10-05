#include "TRUY_XUAT/TruyXuat.h"
#include "XOA_BENH_NHAN/XoaBenhNhan.h"
#include <iostream>
#include <limits>

using namespace std;

int main() {
    QuanLyHangDoi manager;

    while (true) {
        cout << "\n========== TRUY XUAT BENH NHAN ==========\n";
        cout << "1. Truy xuat va sap xep benh nhan\n";
        cout << "2. Xoa mot benh nhan\n";
        cout << "3. Xoa toan bo benh nhan\n";
        cout << "0. Thoat\n";
        cout << "Lua chon: ";

        int choice = -1;
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Lua chon khong hop le.\n";
            continue;
        }

        if (choice == 0) {
            break;
        }

        if (choice == 1) {
            if (manager.taiVaXuLyBenhNhan()) {
                cout << "Da truy xuat va sap xep du lieu thanh cong!\n";
            } else {
                cerr << "Khong the truy xuat va sap xep du lieu!\n";
            }
        } else if (choice == 2) {
            cout << "Nhap ID benh nhan can xoa: ";
            int patientId = 0;
            if (!(cin >> patientId)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "ID benh nhan khong hop le.\n";
                continue;
            }
            XoaBenhNhan::xoaBenhNhan(patientId);
        } else if (choice == 3) {
            cout << "Ban co chac chan muon xoa toan bo benh nhan? "
                    "Nhap Y de xac nhan: ";
            char confirmation = '\0';
            if (!(cin >> confirmation)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Lua chon khong hop le.\n";
                continue;
            }
            if (confirmation == 'Y' || confirmation == 'y') {
                XoaBenhNhan::xoaTatCaBenhNhan();
            } else {
                cout << "Da huy thao tac xoa.\n";
            }
        } else {
            cout << "Lua chon khong hop le.\n";
        }
    }

    return 0;
}