#include <sqlite3.h>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>

// Chặn ngay khi chương trình thử dùng SQL để lọc/sắp xếp/tổng hợp dữ liệu.
// Chỉ liên kết trong kiểm thử; kiểm tra cả câu lệnh đọc và ghi.
static void checkSql(const char* sql) {
    std::string text = sql ? sql : "";
    for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (text.find("select") != std::string::npos) {
        for (const char* forbidden : {"where", "order by", "join", "group by", "limit", "count(", "max(", "min("}) {
            if (text.find(forbidden) != std::string::npos) {
                std::cerr << "SQL read must only load records: " << sql << '\n';
                std::exit(90);
            }
        }
    }
    if (text.find("where") != std::string::npos) {
        std::cerr << "SQL must not contain WHERE: " << sql << '\n';
        std::exit(90);
    }
}
extern "C" int __real_sqlite3_prepare_v2(sqlite3*, const char*, int, sqlite3_stmt**, const char**);
extern "C" int __wrap_sqlite3_prepare_v2(sqlite3* db, const char* sql, int length,
                                       sqlite3_stmt** statement, const char** tail) {
    checkSql(sql);
    return __real_sqlite3_prepare_v2(db, sql, length, statement, tail);
}
extern "C" int __real_sqlite3_exec(sqlite3*, const char*, int (*)(void*,int,char**,char**), void*, char**);
extern "C" int __wrap_sqlite3_exec(sqlite3* db, const char* sql,
    int (*callback)(void*,int,char**,char**), void* argument, char** error) {
    checkSql(sql);
    return __real_sqlite3_exec(db, sql, callback, argument, error);
}
