#include "DatabaseDangKham.h"

#include <filesystem>
#include <iostream>
#include <algorithm>
#include "../../include/SqliteMemoryTable.h"

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

bool DatabaseDangKham::migrateDoctorState()
{
    if (!sourceTableExists("doctor_state")) return true;
    if (!executeSql("BEGIN IMMEDIATE;", "Loi bat dau chuyen trang thai bac si")) return false;

    bool success = executeSql(
        "INSERT OR IGNORE INTO main.doctor_state (doctor_id, duty_mode, busy_until, busy_reason) "
        "SELECT doctor_id, duty_mode, busy_until, busy_reason FROM source.doctor_state;",
        "Loi chuyen trang thai bac si"
    );
    if (success)
        success = executeSql("DROP TABLE source.doctor_state;", "Loi xoa bang trang thai bac si cu");
    if (success)
        success = executeSql("COMMIT;", "Loi hoan tat chuyen trang thai bac si");
    if (!success) sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
    return success;
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
    MemoryTable::Transaction transaction(db);
    MemoryTable::Table existing;
    if (!transaction || !existing.load(db, "dang_kham")) return false;
    const DsaSearch::HashIdIndex ids(existing.rows, [&](const auto& row) {
        return sqlite3_value_int(row[existing.column("checkin_id")].get());
    });

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
        ;
    )";
    sqlite3_stmt* select = nullptr;
    sqlite3_stmt* insert = nullptr;
    if (sqlite3_prepare_v2(db, selectSql, -1, &select, nullptr) != SQLITE_OK) return false;
    if (sqlite3_prepare_v2(db, insertSql, -1, &insert, nullptr) != SQLITE_OK) {
        sqlite3_finalize(select); return false;
    }

    bool ok = true;
    int readResult;
    while ((readResult = sqlite3_step(select)) == SQLITE_ROW) {
        const int checkinId = sqlite3_column_int(select, 0);
        size_t position;
        if (ids.find(checkinId, position) &&
            !MemoryTable::isNull(existing.rows[position], existing.column("end_time"))) continue;
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
    if (!ok || readResult != SQLITE_DONE) return false;

    // Sau khi da chuyen du lieu, xoa bang cu khoi truyXuat.db.
    return executeSql("DROP TABLE IF EXISTS source.ket_qua_kham;", "Loi xoa bang ket_qua_kham cu") && transaction.commit();
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

    if (!executeSql(
            R"(
                CREATE TABLE IF NOT EXISTS doctor_state (
                    doctor_id TEXT PRIMARY KEY,
                    duty_mode TEXT NOT NULL DEFAULT 'auto',
                    busy_until TEXT,
                    busy_reason TEXT NOT NULL DEFAULT '',
                    overtime_until TEXT
                );
            )",
            "Loi tao bang trang thai bac si"
        ) || !migrateDoctorState())
    {
        return false;
    }

    if (!addColumnIfMissing("doctor_state", "overtime_until", "TEXT")) return false;

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
    {
        MemoryTable::Transaction transaction(db);
        MemoryTable::Table table;
        if (!transaction || !table.load(db, "dang_kham")) return false;
        const int checkin = table.column("checkin_time"), start = table.column("start_time");
        for (auto& row : table.rows) {
            if (!MemoryTable::text(row, checkin).empty() || MemoryTable::text(row, start).empty()) continue;
            row[checkin] = row[start];
            if (!MemoryTable::insert(db, "dang_kham", table, row, "checkin_id")) return false;
        }
        if (!transaction.commit()) return false;
    }
    if (!migrateLegacyAssignments() || !suaCaTrungBacSi()) return false;
    // Rang buoc toan ven: CASE tao khoa NULL cho ca da ket thuc/chua co bac si.
    MemoryTable::Transaction transaction(db);
    return transaction && executeSql(
        "DROP INDEX IF EXISTS ux_dang_kham_unfinished_doctor;"
        "CREATE UNIQUE INDEX ux_dang_kham_unfinished_doctor ON dang_kham("
        "CASE WHEN end_time IS NULL AND doctor_id IS NOT NULL AND doctor_id <> '' THEN doctor_id END);",
        "Loi bao ve moi bac si chi nhan mot ca"
    ) && transaction.commit();
}

bool DatabaseDangKham::suaCaTrungBacSi() {
    MemoryTable::Transaction transaction(db);
    vector<ExamSession> records;
    MemoryTable::Table table;
    if (!transaction || !docDanhSach(records) || !table.load(db, "dang_kham")) return false;
    const auto duplicates = ExamCore::duplicateDoctorAssignments(records);
    string timestamp;
    if (!MemoryTable::now(db, timestamp)) return false;
    if (!duplicates.empty()) {
        string sql = "CREATE TABLE IF NOT EXISTS dang_kham_assignment_archive (";
        for (const auto& c : table.columns) sql += MemoryTable::identifier(c) + ",";
        sql += "archived_at TEXT, repair_reason TEXT);";
        if (!executeSql(sql, "Loi tao ban luu lich cu")) return false;
    }
    const DsaSearch::HashIdIndex ids(table.rows, [&](const auto& row) {
        return sqlite3_value_int(row[table.column("checkin_id")].get());
    });
    for (int id : duplicates) {
        size_t position;
        if (!ids.find(id, position)) return false;
        auto& row = table.rows[position];
        if (!MemoryTable::isNull(row, table.column("end_time"))) return false;
        auto archived = row;
        MemoryTable::Table archiveTable;
        archiveTable.columns = table.columns;
        archiveTable.columns.push_back("archived_at");
        archiveTable.columns.push_back("repair_reason");
        const string reason = "duplicate_doctor";
        archived.push_back(MemoryTable::value(db, &timestamp));
        archived.push_back(MemoryTable::value(db, &reason));
        if (!archived[archived.size()-2] || !archived.back() ||
            !MemoryTable::insert(db, "dang_kham_assignment_archive", archiveTable, archived)) return false;
        for (const char* c : {"doctor_id", "doctor_name", "doctor_department", "start_time",
                              "planned_end_time", "exam_duration"})
            if (!MemoryTable::set(db, table, row, c, nullptr)) return false;
        if (!MemoryTable::set(db, table, row, "status", string("CHO_DOI")) ||
            !MemoryTable::set(db, table, row, "note", string("Tra ve hang doi do lich cu trung bac si")) ||
            !MemoryTable::set(db, table, row, "updated_at", timestamp) ||
            !MemoryTable::insert(db, "dang_kham", table, row, "checkin_id")) return false;
    }
    return transaction.commit();
}

// DANG_KHAM la nguon assignment duy nhat sau khi bo ket_qua_kham.
namespace {
optional<string> nullableText(sqlite3_stmt* stmt, int column) {
    if (sqlite3_column_type(stmt, column) == SQLITE_NULL) return nullopt;
    const string value = getText(stmt, column);
    if (value.empty()) return nullopt;
    return value;
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

    MemoryTable::Table existing;
    if (!existing.load(db, "dang_kham")) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    const DsaSearch::HashIdIndex ids(existing.rows, [&](const auto& row) {
        return sqlite3_value_int(row[existing.column("checkin_id")].get());
    });
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
        ;
    )";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    bool ok = true;
    for (const auto& bn : assignments) {
        size_t position;
        if (ids.find(bn.CheckinId, position) &&
            !MemoryTable::isNull(existing.rows[position], existing.column("end_time"))) continue;
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
        "FROM dang_kham;";

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
        // Tim tuyen tinh trong du lieu da nap; SQL chi doc toan bo bang.
        if (record.DoctorId.empty()) continue;
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
    MemoryTable::Transaction transaction(db);
    MemoryTable::Table table;
    string timestamp;
    if (!transaction || !table.load(db, "dang_kham") || !MemoryTable::now(db, timestamp)) return false;
    const int doctor = table.column("doctor_id"), end = table.column("end_time"),
              start = table.column("start_time"), id = table.column("checkin_id");
    for (const auto& row : table.rows) {
        if (!MemoryTable::isNull(row, doctor) && MemoryTable::text(row, doctor) == doctorId &&
            MemoryTable::isNull(row, end) &&
            (MemoryTable::isNull(row, start) || MemoryTable::text(row, start) > timestamp)) {
            if (!MemoryTable::erase(db, "dang_kham", "checkin_id", sqlite3_value_int(row[id].get()))) return false;
        }
    }
    return transaction.commit();
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
    MemoryTable::Transaction transaction(db);
    MemoryTable::Table table;
    string timestamp;
    if (!transaction || !table.load(db, "dang_kham") || !MemoryTable::now(db, timestamp)) return false;
    auto* row = table.find("checkin_id", checkinId);
    if (!row || !MemoryTable::isNull(*row, table.column("end_time"))) return false;
    return MemoryTable::set(db, table, *row, "chan_doan", diagnosis) &&
        MemoryTable::set(db, table, *row, "don_thuoc", prescription) &&
        MemoryTable::set(db, table, *row, "loi_nhac_bac_si", reminder) &&
        MemoryTable::set(db, table, *row, "updated_at", timestamp) &&
        MemoryTable::insert(db, "dang_kham", table, *row, "checkin_id") && transaction.commit();
}

// Ghi thời gian kết thúc thực tế; không dùng giờ kết thúc dự kiến để thay thế.
bool DatabaseDangKham::ketThucPhien(int checkinId) {
    MemoryTable::Transaction transaction(db);
    MemoryTable::Table table;
    string timestamp;
    if (!transaction || !table.load(db, "dang_kham") || !MemoryTable::now(db, timestamp)) return false;
    auto* row = table.find("checkin_id", checkinId);
    if (!row || !MemoryTable::isNull(*row, table.column("end_time"))) return false;
    return MemoryTable::set(db, table, *row, "end_time", timestamp) &&
        MemoryTable::set(db, table, *row, "updated_at", timestamp) &&
        MemoryTable::insert(db, "dang_kham", table, *row, "checkin_id") && transaction.commit();
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
