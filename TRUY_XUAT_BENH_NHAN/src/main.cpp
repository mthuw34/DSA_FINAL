#include "QuanLyHangDoi.h"
#include <iostream>

using namespace std;

int main() {
    QuanLyHangDoi manager;
    
    if (manager.taiVaXuLyBenhNhan()) {
        cout << "Da truy xuat va sap xep du lieu thanh cong!\n";
    } else {
        cerr << "khong the truy xuat và sap xep du lieu!\n";
        return 1;
    }

    return 0;
}