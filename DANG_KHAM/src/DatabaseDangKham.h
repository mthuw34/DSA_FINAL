#pragma once

#include <string>
#include <sqlite3.h>
using namespace std;
class DatabaseDangKham
{
private:
    sqlite3* db = nullptr;

    bool executeSql(
        const string& sql,
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
    ~DatabaseDangKham();

    bool mo(
        const string& sourcePath,
        const string& destinationPath
    );

    bool taoCauTruc();

    bool dongBoTuXepBacSi();

    void dong();

    sqlite3* get() const;
};
