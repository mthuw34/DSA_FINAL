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

    // Ghi ket qua phan bac si truc tiep vao DANG_KHAM, khong qua ket_qua_kham.
    bool ghiPhanBacSi(const std::vector<BenhNhanKham>& assignments);

    // Tra cac ca chua bat dau ve hang doi khi bac si bi khoa/bận.
    bool xoaCaChuaBatDauCuaBacSi(const std::string& doctorId);

    bool docDanhSach(std::vector<ExamSession>& records);
    bool luuChanDoan(int checkinId, const std::string& diagnosis,
                    const std::string& prescription, const std::string& reminder);
    bool ketThucPhien(int checkinId);

    void dong();

    sqlite3* get() const;
};
