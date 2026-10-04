#include "XuLyDatabase.h"
#include <iostream>


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

        std::cerr << "Khong mo duoc database: " << duongDan << "\n";

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
    std::vector<BenhNhanKham>& danhSach
) {
    if (!Database) return false;

    danhSach.clear();

    // Chi co MỘT sqlite3_prepare_v2 / MỘT sqlite3_step loop.
    // UNION ALL gom 10 bang queue thanh mot tap du lieu duy nhat.
    // Tuyet doi khong WHERE va khong ORDER BY.
    std::string sql =
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

bool XuLyDatabase::GhiKetQua(const BenhNhanKham& bn) {
    if (!Database) return false;

    const char* createSql = R"(
        CREATE TABLE IF NOT EXISTS ket_qua_kham (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            checkin_id INTEGER NOT NULL UNIQUE,
            patient_id INTEGER NOT NULL,
            khoa_benh_nhan TEXT NOT NULL,
            khoa_bac_si TEXT NOT NULL,
            doctor_id TEXT NOT NULL,
            doctor_name TEXT NOT NULL,
            start_time TEXT NOT NULL,
            exam_duration INTEGER NOT NULL,
            end_time TEXT NOT NULL,
            Status TEXT NOT NULL,
            Note TEXT
        );
    )";

    char* error = nullptr;

    if (sqlite3_exec(
            Database,
            createSql,
            nullptr,
            nullptr,
            &error
        ) != SQLITE_OK) {

        std::cerr << "Loi tao bang ket qua: "
                  << (error ? error : "") << "\n";
        sqlite3_free(error);
        return false;
    }

    const char* insertSql = R"(
        INSERT OR REPLACE INTO ket_qua_kham (
            checkin_id,
            patient_id,
            khoa_benh_nhan,
            khoa_bac_si,
            doctor_id,
            doctor_name,
            start_time,
            exam_duration,
            end_time,
            Status,
            Note
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            Database,
            insertSql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, bn.CheckinId);
    sqlite3_bind_int(stmt, 2, bn.PatientId);
    sqlite3_bind_text(stmt, 3, bn.khoa.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, bn.KhoaBacSi.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, bn.DoctorId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, bn.DoctorName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, bn.StartTime.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, bn.ExamDuration);
    sqlite3_bind_text(stmt, 9, bn.EndTime.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, bn.Status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, bn.Note.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    return success;
}

bool XuLyDatabase::GhiKetQuaNhieu(
    const std::vector<BenhNhanKham>& DanhSach
) {
    if (!Database) return false;
    if (DanhSach.empty()) return true;

    char* error = nullptr;
    if (sqlite3_exec(
            Database,
            "BEGIN TRANSACTION;",
            nullptr,
            nullptr,
            &error
        ) != SQLITE_OK) {
        if (error) sqlite3_free(error);
        return false;
    }

    for (const BenhNhanKham& bn : DanhSach) {
        if (!GhiKetQua(bn)) {
            sqlite3_exec(
                Database,
                "ROLLBACK;",
                nullptr,
                nullptr,
                nullptr
            );
            if (error) sqlite3_free(error);
            return false;
        }
    }

    if (sqlite3_exec(
            Database,
            "COMMIT;",
            nullptr,
            nullptr,
            &error
        ) != SQLITE_OK) {
        if (error) sqlite3_free(error);
        return false;
    }

    if (error) sqlite3_free(error);
    return true;
}

sqlite3* XuLyDatabase::LayDatabase() const {
    return Database;
}
