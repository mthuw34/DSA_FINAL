#pragma once

#include <string>
#include <sqlite3.h>
#include "ExamCore.h"
#include "BenhNhanKham.h"
#include <vector>
class DatabaseDangKham
{
private:
    sqlite3* db = nullptr;

    bool executeSql(
        const std::string& sql,
        const char* errorMessage
    );

    bool columnExists(
        const char* table,
        const char* column
    );

    bool addColumnIfMissing(
        const char* table,
        const char* column,
        const char* definition
    );

    bool sourceTableExists(
        const char* tableName
    );

    bool migrateLegacyAssignments();
    bool migrateDoctorState();

public:
    DatabaseDangKham() = default;
    DatabaseDangKham(const DatabaseDangKham&) = delete;
    DatabaseDangKham& operator=(const DatabaseDangKham&) = delete;
    ~DatabaseDangKham();

    bool mo(
        const std::string& sourcePath,
        const std::string& destinationPath
    );

    bool taoCauTruc();

    bool dongBoTuXepBacSi(const std::vector<std::string>& blockedDoctors = {});

    // Ghi kết quả phân bác sĩ trực tiếp vào DANG_KHAM.
    bool ghiPhanBacSi(const std::vector<BenhNhanKham>& assignments);

    // Đọc các lượt đã được phân bác sĩ trực tiếp từ dangKham.db.
    bool docPhanBacSi(std::vector<BenhNhanKham>& assignments);

    // Trả các ca chưa bắt đầu về hàng đợi khi bác sĩ bị khóa/bận.
    bool xoaCaChuaBatDauCuaBacSi(const std::string& doctorId);

    bool docDanhSach(std::vector<ExamSession>& records);
    bool luuChanDoan(int checkinId, const std::string& diagnosis,
                    const std::string& prescription, const std::string& reminder);
    bool ketThucPhien(int checkinId);

    void dong();

    sqlite3* get() const;
};
