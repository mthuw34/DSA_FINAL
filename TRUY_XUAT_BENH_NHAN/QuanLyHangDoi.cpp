#include "QuanLyHangDoi.h"
#include "HoSoTruyXuat.h"
#include "ThuatToanSapXep.h"
#include "XuLyDuLieu.h"
#include <vector>
#include <sqlite3.h>

using namespace std;

QuanLyHangDoi::QuanLyHangDoi() {
    daXuLyThanhCong = false;
    taiVaXuLyBenhNhan();
}

bool QuanLyHangDoi::xuLyThanhCong() const {
    return daXuLyThanhCong;
}

void QuanLyHangDoi::taiVaXuLyBenhNhan() {
    const char* databasePath = "QUAN_LY_BENH_NHAN/hospital.db";
    sqlite3* database = nullptr;

    // Mở file DB gốc
    if (sqlite3_open_v2(databasePath, &database, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        if (database != nullptr) {
            sqlite3_close(database);
        }
        return;
    }

    vector<HoSoTruyXuat> records;
    
    // 1. Lấy dữ liệu từ DB
    if (XuLyDuLieu::layDanhSachBenhNhan(database, records)) {
        
        // 2. Thuật toán Merge Sort sắp xếp dữ liệu
        if (!records.empty()) {
            vector<HoSoTruyXuat> buffer(records.size());
            ThuatToanSapXep::sapXepTron(records, buffer, 0, static_cast<int>(records.size()) - 1);
        }
        
        // 3. Đổ dữ liệu đã sort vào truyXuat.db
        const char* outputPath = "TRUY_XUAT_BENH_NHAN/truyXuat.db";
        daXuLyThanhCong = XuLyDuLieu::xuatDuLieuDaSapXep(records, outputPath);
    }

    // Đóng file DB gốc
    sqlite3_close(database);
}