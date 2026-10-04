#include "TruyXuat.h"
#include <vector>
#include <iostream>

using namespace std;

bool QuanLyHangDoi::taiVaXuLyBenhNhan() {
    vector<HoSoTruyXuat> records;
    if (!DBTruyXuat::docDanhSachBenhNhan(records)) {
        cerr << "Khong the doc danh sach benh nhan tu database.\n";
        return false;
    }

    if (!records.empty()) {
        // Merge Sort uses O(n) auxiliary storage to guarantee stable sorting in Theta(n log n) time.
        vector<HoSoTruyXuat> buffer(records.size());
        ThuatToanSapXep::sapXepTron(records, buffer, 0, static_cast<int>(records.size()) - 1);
    }

    return DBTruyXuat::ghiDanhSachDaSapXep(records);
}