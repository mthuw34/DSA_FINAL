#include "QuanLyHangDoi.h"
#include "HoSoTruyXuat.h"
#include "ThuatToanSapXep.h"
#include "XuLyDuLieu.h"
#include <vector>
#include <sqlite3.h>

using namespace std;

bool QuanLyHangDoi::taiVaXuLyBenhNhan() {
    const char* hospitalPath = "QUAN_LY_BENH_NHAN/hospital.db";
    const char* priorityPath = "THAY_DOI_MUC_DO_UU_TIEN/priority.db";
    sqlite3* hospitalDatabase = nullptr;
    sqlite3* priorityDatabase = nullptr;

    // Kiểm tra mở DB hospital
    if (sqlite3_open_v2(hospitalPath, &hospitalDatabase, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        if (hospitalDatabase != nullptr) {
            sqlite3_close(hospitalDatabase);
        }
        return false; // Trả về false nếu lỗi
    }

    // Kiểm tra mở DB priority
    if (sqlite3_open_v2(priorityPath, &priorityDatabase, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        sqlite3_close(hospitalDatabase);
        if (priorityDatabase != nullptr) {
            sqlite3_close(priorityDatabase);
        }
        return false; // Trả về false nếu lỗi
    }

    vector<HoSoTruyXuat> records;
    bool success = false;
    
    // 1. Lấy dữ liệu từ cả 2 DB
    if (XuLyDuLieu::layDanhSachBenhNhan(hospitalDatabase, priorityDatabase, records)) {
        
        // 2. Thuật toán Merge Sort sắp xếp dữ liệu
        if (!records.empty()) {
            vector<HoSoTruyXuat> buffer(records.size());
            ThuatToanSapXep::sapXepTron(records, buffer, 0, static_cast<int>(records.size()) - 1);
        }
        
        // 3. Đổ dữ liệu đã sort vào truyXuat.db và lưu lại trạng thái thành công
        const char* outputPath = "TRUY_XUAT_BENH_NHAN/truyXuat.db";
        success = XuLyDuLieu::xuatDuLieuDaSapXep(records, outputPath);
    }

    sqlite3_close(priorityDatabase);
    sqlite3_close(hospitalDatabase);
    
    return success; // Trả về true nếu thành công từ đầu đến cuối
}