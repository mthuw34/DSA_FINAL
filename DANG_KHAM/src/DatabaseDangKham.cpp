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
                    start_time TEXT,
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
    return success;
}

namespace {
// Giữ NULL của dữ liệu cũ để phân biệt ca chưa kết thúc và trường chưa nhập.
optional<string> nullableText(sqlite3_stmt* stmt, int column) {
    if (sqlite3_column_type(stmt, column) == SQLITE_NULL) return nullopt;
    return getText(stmt, column);
}

void bindText(sqlite3_stmt* stmt, int column, const optional<string>& text) {
    if (text) sqlite3_bind_text(stmt, column, text->c_str(), -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(stmt, column);
}

// Nạp toàn bộ kết quả phân bác sĩ; tầng ExamCore quyết định ca cần nhận.
bool loadAssignments(sqlite3* db, vector<ExamAssignment>& records) {
    records.clear();
    const char* sql = "SELECT checkin_id, patient_id, khoa_bac_si, doctor_id, doctor_name, "
                      "start_time, end_time, Status FROM source.ket_qua_kham;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    int result;
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        ExamAssignment assignment;
        assignment.session.checkinId = sqlite3_column_int(stmt, 0);
        assignment.session.patientId = sqlite3_column_int(stmt, 1);
        assignment.session.department = getText(stmt, 2);
        assignment.session.doctorId = nullableText(stmt, 3);
        assignment.session.doctorName = nullableText(stmt, 4);
        assignment.session.startTime = nullableText(stmt, 5);
        assignment.plannedEnd = nullableText(stmt, 6);
        assignment.status = getText(stmt, 7);
        records.push_back(assignment);
    }
    sqlite3_finalize(stmt);
    return result == SQLITE_DONE;
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
bool DatabaseDangKham::dongBoTuXepBacSi(const vector<string>& blockedDoctors) {
    if (!sourceTableExists("ket_qua_kham")) {
        cerr << "Chua co bang ket_qua_kham trong truyXuat.db. Hay chay SAP_XEP_BAC_SI truoc.\n";
        return false;
    }
    if (!executeSql("BEGIN IMMEDIATE;", "Loi bat dau dong bo")) return false;
    vector<ExamAssignment> assignments;
    vector<ExamSession> existing;
    bool success = loadAssignments(db, assignments) && docDanhSach(existing);
    assignments.erase(remove_if(assignments.begin(), assignments.end(), [&](const ExamAssignment& a) {
        return !ExamCore::findActive(existing, a.session.checkinId) && a.session.doctorId &&
            find(blockedDoctors.begin(), blockedDoctors.end(), *a.session.doctorId) != blockedDoctors.end();
    }), assignments.end());
    stable_sort(assignments.begin(), assignments.end(), [](const ExamAssignment& a, const ExamAssignment& b) {
        return ExamCore::startsBefore(a.session, b.session);
    });
    sqlite3_stmt* insert = nullptr;
    sqlite3_stmt* update = nullptr;
    const char* insertSql = "INSERT INTO dang_kham (checkin_id, patient_id, department, checkin_time, "
        "doctor_id, doctor_name, start_time, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, datetime('now','localtime'));";
    // Điều kiện ghi chỉ bảo vệ bản ghi nếu phiên khác đã kết thúc khám sau khi nạp.
    const char* updateSql = "UPDATE dang_kham SET patient_id=?, department=?, doctor_id=?, doctor_name=?, "
        "start_time=? WHERE checkin_id=? AND end_time IS NULL;";
    if (success) success = sqlite3_prepare_v2(db, insertSql, -1, &insert, nullptr) == SQLITE_OK;
    if (success) success = sqlite3_prepare_v2(db, updateSql, -1, &update, nullptr) == SQLITE_OK;
    if (success) {
        const auto changes = ExamCore::synchronizationChanges(assignments, existing, time(nullptr));
        ExamCore::SessionIndex ids;
        for (size_t i = 0; i < existing.size(); ++i) ids.put(existing[i].checkinId, i);
        for (const auto& session : changes) {
            size_t position;
            const bool found = ids.find(session.checkinId, position);
            auto* stmt = found ? update : insert;
            if (found) {
                sqlite3_bind_int(stmt, 1, session.patientId);
                bindText(stmt, 2, session.department); bindText(stmt, 3, session.doctorId);
                bindText(stmt, 4, session.doctorName); bindText(stmt, 5, session.startTime);
                sqlite3_bind_int(stmt, 6, session.checkinId);
            } else {
                sqlite3_bind_int(stmt, 1, session.checkinId); sqlite3_bind_int(stmt, 2, session.patientId);
                bindText(stmt, 3, session.department); bindText(stmt, 4, session.checkinTime);
                bindText(stmt, 5, session.doctorId); bindText(stmt, 6, session.doctorName);
                bindText(stmt, 7, session.startTime);
            }
            if (sqlite3_step(stmt) != SQLITE_DONE) { success = false; break; }
            sqlite3_reset(stmt); sqlite3_clear_bindings(stmt);
        }
    }
    sqlite3_finalize(insert); sqlite3_finalize(update);
    if (success) success = executeSql("COMMIT;", "Loi luu dong bo");
    if (!success) {
        cerr << "Loi dong bo benh nhan dang kham: " << sqlite3_errmsg(db) << '\n';
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
    }
    return success;
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
