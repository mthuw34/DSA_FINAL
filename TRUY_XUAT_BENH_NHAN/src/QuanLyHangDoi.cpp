#include "QuanLyHangDoi.h"
#include "HoSoTruyXuat.h"
#include "ThuatToanSapXep.h"
#include "XuLyDuLieu.h"
#include <vector>
#include <sqlite3.h>

using namespace std;

bool QuanLyHangDoi::taiVaXuLyBenhNhan() {
    const char* priorityPath = "THAY_DOI_MUC_DO_UU_TIEN/priority.db";
    sqlite3* priorityDatabase = nullptr;

    if (sqlite3_open_v2(priorityPath, &priorityDatabase, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        if (priorityDatabase != nullptr) {
            sqlite3_close(priorityDatabase);
        }
        return false;
    }

    vector<HoSoTruyXuat> records;
    bool success = false;
    
    // 1. Lấy dữ liệu trực tiếp từ priority.db
    if (XuLyDuLieu::layDanhSachBenhNhan(priorityDatabase, records)) {
        
        // 2. Thuật toán Merge Sort sắp xếp dữ liệu
        if (!records.empty()) {
            vector<HoSoTruyXuat> buffer(records.size());
            ThuatToanSapXep::sapXepTron(records, buffer, 0, static_cast<int>(records.size()) - 1);
        }
        
        // 3. Đổ dữ liệu đã sort vào truyXuat.db và lưu lại trạng thái thành công
        const char* outputPath = "TRUY_XUAT_BENH_NHAN/db/truyXuat.db";
        success = XuLyDuLieu::xuatDuLieuDaSapXep(records, outputPath);
    }

    sqlite3_close(priorityDatabase);

    return success;
}