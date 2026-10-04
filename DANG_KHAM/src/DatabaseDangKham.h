#pragma once

#include <string>
#include <sqlite3.h>
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

    bool dongBoTuXepBacSi();

    void dong();

    sqlite3* get() const;
};
