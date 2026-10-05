#include "DatabaseDangKham.h"

#include <filesystem>
#include <iostream>
#include <algorithm>

using namespace std;

// Lấy chuỗi TEXT an toàn từ SQLite.
static string getText(
    sqlite3_stmt* stmt,
    int column
)
{
    const unsigned char* text =
        sqlite3_column_text(stmt, column);

    return text
        ? reinterpret_cast<const char*>(text)
        : "";
}

DatabaseDangKham::~DatabaseDangKham()
{
    dong();
}

// Thực thi một câu lệnh SQL không cần trả về dòng dữ liệu.
bool DatabaseDangKham::executeSql(
    const string& sql,
    const char* errorMessage
)
{
    char* error = nullptr;

    if (sqlite3_exec(
            db,
            sql.c_str(),
            nullptr,
            nullptr,
            &error
        ) == SQLITE_OK)
    {
        return true;
    }

    cerr
        << errorMessage
        << ": "
        << (error ? error : sqlite3_errmsg(db))
        << '\n';

    sqlite3_free(error);
    return false;
}

// Kiểm tra một cột đã tồn tại trong bảng hay chưa.
bool DatabaseDangKham::columnExists(
    const char* table,
    const char* column
)
{
    const string sql =
        "PRAGMA table_info(" +
        string(table) +
        ");";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            sql.c_str(),
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }

    bool found = false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (getText(stmt, 1) == column)
        {
            found = true;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return found;
}

// Bổ sung cột vào bảng nếu database cũ chưa có cột đó.
bool DatabaseDangKham::addColumnIfMissing(
    const char* table,
    const char* column,
    const char* definition
)
{
    if (columnExists(table, column))
        return true;

    const string sql =
        "ALTER TABLE " +
        string(table) +
        " ADD COLUMN " +
        column +
        " " +
        definition +
        ";";

    return executeSql(
        sql,
        "Loi bo sung cot"
    );
}

// Nạp thông tin bảng rồi kiểm tra tên trong bộ nhớ.
bool DatabaseDangKham::sourceTableExists(const char* tableName)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT type, name FROM source.sqlite_master;",
                          -1, &stmt, nullptr) != SQLITE_OK) return false;
    vector<pair<string, string>> tables;
    int result;
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
        tables.emplace_back(getText(stmt, 0), getText(stmt, 1));
    sqlite3_finalize(stmt);
    if (result != SQLITE_DONE) return false;
    for (const auto& table : tables)
        if (table.first == "table" && table.second == tableName) return true;
    return false;
}

// Mở dangKham.db và liên kết database nguồn.
bool DatabaseDangKham::mo(
    const string& sourcePath,
    const string& destinationPath
)
{
    if (sqlite3_open_v2(
            destinationPath.c_str(),
            &db,
            SQLITE_OPEN_READWRITE |
            SQLITE_OPEN_CREATE,
            nullptr
        ) != SQLITE_OK)
    {
        cerr
            << "Khong mo duoc dangKham.db: "
            << (db ? sqlite3_errmsg(db) : "loi SQLite")
            << '\n';

        dong();
        return false;
    }

    sqlite3_busy_timeout(db, 5000);

    const string absoluteSource =
        filesystem::absolute(sourcePath)
            .string();

    sqlite3_stmt* stmt = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        "ATTACH DATABASE ? AS source;",
        -1,
        &stmt,
        nullptr
    );

    if (result == SQLITE_OK)
    {
        result = sqlite3_bind_text(
            stmt,
            1,
            absoluteSource.c_str(),
            -1,
            SQLITE_TRANSIENT
        );
    }

    if (result == SQLITE_OK)
        result = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (result != SQLITE_DONE)
    {
        cerr
            << "Loi lien ket truyXuat.db: "
            << sqlite3_errmsg(db)
            << '\n';

        dong();
        return false;
    }

    return true;
}

bool DatabaseDangKham::migrateLegacyAssignments() {
    if (!sourceTableExists("ket_qua_kham")) return true;

    const char* selectSql = R"(
        SELECT checkin_id, patient_id, khoa_benh_nhan, khoa_bac_si,
               doctor_id, doctor_name, start_time, exam_duration,
               end_time, Status, Note
        FROM source.ket_qua_kham;
    )";
    const char* insertSql = R"(
        INSERT INTO dang_kham (
            checkin_id, patient_id, department, checkin_time,
            doctor_id, doctor_name, doctor_department, start_time, planned_end_time,
            exam_duration, status, note, updated_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now','localtime'))
        ON CONFLICT(checkin_id) DO UPDATE SET
            patient_id=excluded.patient_id,
            department=excluded.department,
            doctor_id=excluded.doctor_id,
            doctor_name=excluded.doctor_name,
            doctor_department=excluded.doctor_department,
            start_time=excluded.start_time,
            planned_end_time=excluded.planned_end_time,
            exam_duration=excluded.exam_duration,
            status=excluded.status,
            note=excluded.note,
            updated_at=datetime('now','localtime')
        WHERE dang_kham.end_time IS NULL;
    )";
    sqlite3_stmt* select = nullptr;
    sqlite3_stmt* insert = nullptr;
    if (sqlite3_prepare_v2(db, selectSql, -1, &select, nullptr) != SQLITE_OK) return false;
    if (sqlite3_prepare_v2(db, insertSql, -1, &insert, nullptr) != SQLITE_OK) {
        sqlite3_finalize(select); return false;
    }

    bool ok = true;
    while (sqlite3_step(select) == SQLITE_ROW) {
        const int checkinId = sqlite3_column_int(select, 0);
        const int patientId = sqlite3_column_int(select, 1);
        const string department = getText(select, 2);
        const string doctorDepartment = getText(select, 3);
        const string doctorId = getText(select, 4);
        const string doctorName = getText(select, 5);
        const string startTime = getText(select, 6);
        const int duration = sqlite3_column_int(select, 7);
        const string plannedEnd = getText(select, 8);
        const string status = getText(select, 9);
        const string note = getText(select, 10);

        time_t parsedStart = 0;
        time_t parsedEnd = 0;
        if (status != "DA_XEP_BAC_SI" ||
            !ExamCore::parseTime(startTime, parsedStart) ||
            !ExamCore::parseTime(plannedEnd, parsedEnd) ||
            parsedEnd <= parsedStart) {
            continue;
        }

        sqlite3_bind_int(insert, 1, checkinId);
        sqlite3_bind_int(insert, 2, patientId);
        sqlite3_bind_text(insert, 3, department.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert, 4, startTime.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert, 5, doctorId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert, 6, doctorName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert, 7, doctorDepartment.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert, 8, startTime.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert, 9, plannedEnd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(insert, 10, duration);
        sqlite3_bind_text(insert, 11, status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert, 12, note.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(insert) != SQLITE_DONE) { ok = false; break; }
        sqlite3_reset(insert); sqlite3_clear_bindings(insert);
    }
    sqlite3_finalize(select);
    sqlite3_finalize(insert);
    if (!ok) return false;

    // Sau khi da chuyen du lieu, xoa bang cu khoi truyXuat.db.
    return executeSql("DROP TABLE IF EXISTS source.ket_qua_kham;", "Loi xoa bang ket_qua_kham cu");
}

// Tạo và cập nhật cấu trúc bảng dang_kham.
bool DatabaseDangKham::taoCauTruc()
{
    if (!executeSql(
            R"(
                CREATE TABLE IF NOT EXISTS dang_kham (
                    checkin_id INTEGER PRIMARY KEY,
                    patient_id INTEGER NOT NULL,
                    department TEXT NOT NULL,
                    checkin_time TEXT NOT NULL,
                    doctor_id TEXT,
                    doctor_name TEXT,
                    doctor_department TEXT,
                    start_time TEXT,
                    planned_end_time TEXT,
                    exam_duration INTEGER,
                    status TEXT,
                    note TEXT,
                    end_time TEXT,
                    chan_doan TEXT,
                    don_thuoc TEXT,
                    loi_nhac_bac_si TEXT,
                    updated_at TEXT
                );
            )",
            "Loi tao bang dang_kham"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "checkin_time",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "doctor_id",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "doctor_name",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "start_time",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing("dang_kham", "doctor_department", "TEXT")) return false;

    if (!addColumnIfMissing("dang_kham", "planned_end_time", "TEXT")) return false;
    if (!addColumnIfMissing("dang_kham", "exam_duration", "INTEGER")) return false;
    if (!addColumnIfMissing("dang_kham", "status", "TEXT")) return false;
    if (!addColumnIfMissing("dang_kham", "note", "TEXT")) return false;

    if (!addColumnIfMissing(
            "dang_kham",
            "end_time",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "chan_doan",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "don_thuoc",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "loi_nhac_bac_si",
            "TEXT"
        ))
    {
        return false;
    }

    if (!addColumnIfMissing(
            "dang_kham",
            "updated_at",
            "TEXT"
        ))
    {
        return false;
    }

    // Bổ sung giờ còn thiếu bằng cách duyệt bản ghi trong bộ nhớ.
    vector<ExamSession> records;
    if (!docDanhSach(records)) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "UPDATE dang_kham SET checkin_time = ? WHERE checkin_id = ?;",
                          -1, &stmt, nullptr) != SQLITE_OK) return false;
    bool success = true;
    for (const auto& session : records) {
        if (session.checkinTime || !session.startTime) continue;
        sqlite3_bind_text(stmt, 1, session.startTime->c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, session.checkinId);
        if (sqlite3_step(stmt) != SQLITE_DONE) { success = false; break; }
        sqlite3_reset(stmt); sqlite3_clear_bindings(stmt);
    }
    sqlite3_finalize(stmt);
    if (!success) return false;
    return migrateLegacyAssignments();
}

// DANG_KHAM la nguon assignment duy nhat sau khi bo ket_qua_kham.
namespace {
optional<string> nullableText(sqlite3_stmt* stmt, int column) {
    if (sqlite3_column_type(stmt, column) == SQLITE_NULL) return nullopt;
    return getText(stmt, column);
}

void bindText(sqlite3_stmt* stmt, int column, const optional<string>& text) {
    if (text) sqlite3_bind_text(stmt, column, text->c_str(), -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(stmt, column);
}
}

// Nạp tất cả phiên khám, không lọc trạng thái hoặc sắp thứ tự bằng SQL.
bool DatabaseDangKham::docDanhSach(vector<ExamSession>& records) {
    records.clear();
    const char* sql = "SELECT checkin_id, patient_id, department, checkin_time, doctor_id, doctor_name, "
                      "start_time, end_time, chan_doan, don_thuoc, loi_nhac_bac_si, updated_at FROM dang_kham;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    int result;
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        ExamSession session;
        session.checkinId = sqlite3_column_int(stmt, 0); session.patientId = sqlite3_column_int(stmt, 1);
        session.department = getText(stmt, 2); session.checkinTime = nullableText(stmt, 3);
        session.doctorId = nullableText(stmt, 4); session.doctorName = nullableText(stmt, 5);
        session.startTime = nullableText(stmt, 6); session.endTime = nullableText(stmt, 7);
        session.diagnosis = nullableText(stmt, 8); session.prescription = nullableText(stmt, 9);
        session.reminder = nullableText(stmt, 10); session.updatedAt = nullableText(stmt, 11);
        records.push_back(session);
    }
    sqlite3_finalize(stmt);
    if (result != SQLITE_DONE) records.clear();
    return result == SQLITE_DONE;
}

// Nhận thay đổi do tầng C++ đã tính và ghi từng bản ghi trong một giao dịch.
bool DatabaseDangKham::ghiPhanBacSi(const vector<BenhNhanKham>& assignments) {
    if (!executeSql("BEGIN IMMEDIATE;", "Loi bat dau ghi phan bac si")) return false;

    const char* sql = R"(
        INSERT INTO dang_kham (checkin_id, patient_id, department, checkin_time,
            doctor_id, doctor_name, doctor_department, start_time, planned_end_time, exam_duration, status, note, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now','localtime'))
        ON CONFLICT(checkin_id) DO UPDATE SET
            patient_id=excluded.patient_id, department=excluded.department,
            doctor_id=excluded.doctor_id, doctor_name=excluded.doctor_name,
            doctor_department=excluded.doctor_department, start_time=excluded.start_time, planned_end_time=excluded.planned_end_time,
            exam_duration=excluded.exam_duration, status=excluded.status, note=excluded.note,
            updated_at=datetime('now','localtime')
        WHERE dang_kham.end_time IS NULL;
    )";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    bool ok = true;
    for (const auto& bn : assignments) {
        sqlite3_bind_int(stmt, 1, bn.CheckinId);
        sqlite3_bind_int(stmt, 2, bn.PatientId);
        bindText(stmt, 3, bn.khoa);
        bindText(stmt, 4, bn.CheckinTime);
        bindText(stmt, 5, bn.DoctorId);
        bindText(stmt, 6, bn.DoctorName);
        bindText(stmt, 7, bn.KhoaBacSi);
        bindText(stmt, 8, bn.StartTime);
        bindText(stmt, 9, bn.EndTime);
        sqlite3_bind_int(stmt, 10, bn.ExamDuration);
        bindText(stmt, 11, bn.Status);
        bindText(stmt, 12, bn.Note);
        if (sqlite3_step(stmt) != SQLITE_DONE) { ok = false; break; }
        sqlite3_reset(stmt); sqlite3_clear_bindings(stmt);
    }
    sqlite3_finalize(stmt);
    if (ok) ok = executeSql("COMMIT;", "Loi commit phan bac si");
    if (!ok) sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
    return ok;
}

// Đọc lịch phân bác sĩ trực tiếp từ dangKham.db.
bool DatabaseDangKham::docPhanBacSi(vector<BenhNhanKham>& assignments) {
    assignments.clear();

    const char* sql =
        "SELECT checkin_id, patient_id, department, doctor_department, "
        "doctor_id, doctor_name, start_time, exam_duration, planned_end_time, "
        "status, note, checkin_time "
        "FROM dang_kham "
        "WHERE doctor_id IS NOT NULL AND doctor_id <> '';";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    int result;
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        BenhNhanKham record;
        record.CheckinId = sqlite3_column_int(stmt, 0);
        record.PatientId = sqlite3_column_int(stmt, 1);
        record.khoa = getText(stmt, 2);
        record.KhoaBacSi = getText(stmt, 3);
        record.DoctorId = getText(stmt, 4);
        record.DoctorName = getText(stmt, 5);
        record.StartTime = getText(stmt, 6);
        record.ExamDuration = sqlite3_column_int(stmt, 7);
        record.EndTime = getText(stmt, 8);
        record.Status = getText(stmt, 9);
        record.Note = getText(stmt, 10);
        record.CheckinTime = getText(stmt, 11);
        assignments.push_back(record);
    }

    sqlite3_finalize(stmt);
    if (result != SQLITE_DONE) {
        assignments.clear();
        return false;
    }

    return true;
}

bool DatabaseDangKham::xoaCaChuaBatDauCuaBacSi(const string& doctorId) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM dang_kham WHERE doctor_id=? AND end_time IS NULL "
                      "AND (start_time IS NULL OR start_time > datetime('now','localtime'));";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, doctorId.c_str(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool DatabaseDangKham::dongBoTuXepBacSi(const vector<string>& blockedDoctors) {
    // Scheduler da ghi assignment truc tiep vao DANG_KHAM.
    // O buoc dong bo, chi can tra cac ca chua bat dau cua bac si dang bi khoa.
    for (const auto& doctorId : blockedDoctors) {
        if (!xoaCaChuaBatDauCuaBacSi(doctorId)) return false;
    }
    return true;
}

// Lưu chẩn đoán cho ca đã được tầng xử lý tìm và kiểm tra trong bộ nhớ.
bool DatabaseDangKham::luuChanDoan(int checkinId, const string& diagnosis,
                                 const string& prescription, const string& reminder) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE dang_kham SET chan_doan=?, don_thuoc=?, loi_nhac_bac_si=?, "
        "updated_at=datetime('now','localtime') WHERE checkin_id=? AND end_time IS NULL;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    bindText(stmt, 1, diagnosis); bindText(stmt, 2, prescription); bindText(stmt, 3, reminder);
    sqlite3_bind_int(stmt, 4, checkinId);
    const bool success = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) > 0;
    sqlite3_finalize(stmt);
    return success;
}

// Ghi thời gian kết thúc thực tế; không dùng giờ kết thúc dự kiến để thay thế.
bool DatabaseDangKham::ketThucPhien(int checkinId) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE dang_kham SET end_time=datetime('now','localtime'), "
        "updated_at=datetime('now','localtime') WHERE checkin_id=? AND end_time IS NULL;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, checkinId);
    const bool success = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) > 0;
    sqlite3_finalize(stmt);
    return success;
}

// Ngắt liên kết database nguồn và đóng kết nối SQLite.
void DatabaseDangKham::dong()
{
    if (db == nullptr)
        return;

    sqlite3_exec(
        db,
        "DETACH DATABASE source;",
        nullptr,
        nullptr,
        nullptr
    );

    sqlite3_close(db);
    db = nullptr;
}

// Trả về kết nối SQLite để các lớp nghiệp vụ sử dụng.
sqlite3* DatabaseDangKham::get() const
{
    return db;
}
