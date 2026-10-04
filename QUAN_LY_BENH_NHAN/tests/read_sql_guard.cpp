#include <sqlite3.h>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>

// Chặn ngay khi chương trình thử dùng SQL để lọc/sắp xếp/tổng hợp dữ liệu.
// Chỉ liên kết trong kiểm thử; các lệnh ghi theo ID vẫn được cho phép.
extern "C" int __real_sqlite3_prepare_v2(sqlite3*, const char*, int, sqlite3_stmt**, const char**);
extern "C" int __wrap_sqlite3_prepare_v2(sqlite3* db, const char* sql, int length,
                                       sqlite3_stmt** statement, const char** tail) {
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
    return __real_sqlite3_prepare_v2(db, sql, length, statement, tail);
}
