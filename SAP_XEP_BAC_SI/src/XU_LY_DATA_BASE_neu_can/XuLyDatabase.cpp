#include "XuLyDatabase.h"
#include <iostream>
using namespace std;


XuLyDatabase::~XuLyDatabase() {
    DongDatabase();
}

bool XuLyDatabase::MoDatabase(const std::string& duongDan) {
    if (sqlite3_open_v2(
            duongDan.c_str(),
            &Database,
            SQLITE_OPEN_READWRITE,
            nullptr
        ) != SQLITE_OK) {

        cerr << "Khong mo duoc database: " << duongDan << "\n";

        if (Database) {
            sqlite3_close(Database);
            Database = nullptr;
        }
        return false;
    }

    return true;
}

void XuLyDatabase::DongDatabase() {
    if (Database) {
        sqlite3_close(Database);
        Database = nullptr;
    }
}

bool XuLyDatabase::LayTatCaBenhNhan(
    vector<BenhNhanKham>& danhSach
) {
    if (!Database) return false;

    danhSach.clear();

    // union all để gom 10 bảng thành một tập dữ liệu duy nhất
    string sql =
        "SELECT retrieval_order, checkin_id, patient_id, department, "
        "checkin_time, base_priority, current_priority, last_update "
        "FROM queue_khoa_cap_cuu "
        "UNION ALL SELECT retrieval_order, checkin_id, patient_id, department, "
        "checkin_time, base_priority, current_priority, last_update "
        "FROM queue_khoa_noi "
        "UNION ALL SELECT retrieval_order, checkin_id, patient_id, department, "
        "checkin_time, base_priority, current_priority, last_update "
        "FROM queue_khoa_ngoai "
        "UNION ALL SELECT retrieval_order, checkin_id, patient_id, department, "
        "checkin_time, base_priority, current_priority, last_update "
        "FROM queue_khoa_tim_mach "
        "UNION ALL SELECT retrieval_order, checkin_id, patient_id, department, "
        "checkin_time, base_priority, current_priority, last_update "
        "FROM queue_khoa_nhi "
        "UNION ALL SELECT retrieval_order, checkin_id, patient_id, department, "
        "checkin_time, base_priority, current_priority, last_update "
        "FROM queue_khoa_san "
        "UNION ALL SELECT retrieval_order, checkin_id, patient_id, department, "
        "checkin_time, base_priority, current_priority, last_update "
        "FROM queue_khoa_tai_mui_hong "
        "UNION ALL SELECT retrieval_order, checkin_id, patient_id, department, "
        "checkin_time, base_priority, current_priority, last_update "
        "FROM queue_khoa_mat "
        "UNION ALL SELECT retrieval_order, checkin_id, patient_id, department, "
        "checkin_time, base_priority, current_priority, last_update "
        "FROM queue_khoa_da_lieu "
        "UNION ALL SELECT retrieval_order, checkin_id, patient_id, department, "
        "checkin_time, base_priority, current_priority, last_update "
        "FROM queue_khoa_than_kinh;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            Database,
            sql.c_str(),
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK) {

        std::cerr << "Khong nap duoc danh sach benh nhan: "
                  << sqlite3_errmsg(Database) << "\n";
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        BenhNhanKham bn;

        bn.RetrievalOrder = sqlite3_column_int(stmt, 0);
        bn.CheckinId = sqlite3_column_int(stmt, 1);
        bn.PatientId = sqlite3_column_int(stmt, 2);

        const unsigned char* department =
            sqlite3_column_text(stmt, 3);
        const unsigned char* checkinTime =
            sqlite3_column_text(stmt, 4);
        const unsigned char* lastUpdate =
            sqlite3_column_text(stmt, 7);

        bn.khoa = department
            ? reinterpret_cast<const char*>(department)
            : "";

        bn.CheckinTime = checkinTime
            ? reinterpret_cast<const char*>(checkinTime)
            : "";

        bn.BasePriority = sqlite3_column_int(stmt, 5);
        bn.CurrentPriority = sqlite3_column_int(stmt, 6);

        bn.LastUpdate = lastUpdate
            ? reinterpret_cast<const char*>(lastUpdate)
            : "";

        bn.Status = "CHO_KHAM";
        danhSach.push_back(bn);
    }

    int rc = sqlite3_errcode(Database);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE) {
        std::cerr << "Loi khi doc danh sach benh nhan.\n";
        return false;
    }

    return true;
}

sqlite3* XuLyDatabase::LayDatabase() const {
    return Database;
}
