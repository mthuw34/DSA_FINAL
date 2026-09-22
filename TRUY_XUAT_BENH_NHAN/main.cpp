#include "QuanLyHangDoi.h"
#include <iostream>

using namespace std;

int main() {
    // Khởi tạo đối tượng quản lý, quá trình lấy dữ liệu và sắp xếp sẽ tự động chạy
    QuanLyHangDoi manager;

    if (!manager.xuLyThanhCong()) {
        cerr << "Khong the tao TRUY_XUAT_BENH_NHAN/truyXuat.db.\n";
        return 1;
    }

    cout << "Da tao TRUY_XUAT_BENH_NHAN/truyXuat.db thanh cong!\n";
    return 0;
}