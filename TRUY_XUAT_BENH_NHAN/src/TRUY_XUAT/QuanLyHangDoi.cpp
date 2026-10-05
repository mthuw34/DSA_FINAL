#include "TruyXuat.h"
#include "MangDongBenhNhan.h"
#include <iostream>

bool QuanLyHangDoi::taiVaXuLyBenhNhan() {
    MangDongBenhNhan records;
    if (!DBTruyXuat::docDanhSachBenhNhan(records)) {
        std::cerr << "Khong the doc danh sach benh nhan tu database.\n";
        return false;
    }

    MangDongBenhNhan buffer;
    for (int index = 0; index < records.size(); ++index)
        buffer.push_back(records[index]);
    if (!records.empty())
        ThuatToanSapXep::sapXepTron(records, buffer, 0, records.size() - 1);

    return DBTruyXuat::ghiDanhSachDaSapXep(records);
}