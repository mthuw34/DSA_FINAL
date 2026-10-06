#pragma once
#include "LinearSearch.h"
#include <sqlite3.h>
#include <memory>
#include <string>
#include <vector>

// SQLite chi doc toan bang. C++ tim ID va chon cac ban ghi can ghi/xoa.
namespace MemoryTable {
using Value = std::shared_ptr<sqlite3_value>;
using Row = std::vector<Value>;
inline std::string identifier(const std::string& name) {
    std::string quoted = "\"";
    for (char c : name) { quoted += c; if (c == '"') quoted += c; }
    return quoted + "\"";
}
inline bool exec(sqlite3* db, const std::string& sql) {
    return sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK;
}

// Khoa ghi truoc khi nap du lieu; rollback ca khi COMMIT that bai.
class Transaction {
    sqlite3* db;
    bool owner, active;
public:
    explicit Transaction(sqlite3* connection) : db(connection),
        owner(sqlite3_get_autocommit(db) != 0),
        active(exec(db, owner ? "BEGIN IMMEDIATE;" : "SAVEPOINT memory_table_write;")) {}
    Transaction(const Transaction&) = delete;
    ~Transaction() {
        if (!active) return;
        if (owner) exec(db, "ROLLBACK;");
        else { exec(db, "ROLLBACK TO memory_table_write;"); exec(db, "RELEASE memory_table_write;"); }
    }
    explicit operator bool() const { return active; }
    bool commit() {
        if (!active || !exec(db, owner ? "COMMIT;" : "RELEASE memory_table_write;")) return false;
        active = false;
        return true;
    }
};

struct Table {
    std::vector<std::string> columns;
    std::vector<Row> rows;
    int column(const std::string& name) const {
        for (std::size_t i = 0; i < columns.size(); ++i)
            if (columns[i] == name) return static_cast<int>(i);
        return -1;
    }
    bool load(sqlite3* db, const std::string& name) {
        columns.clear(); rows.clear();
        sqlite3_stmt* stmt = nullptr;
        const std::string sql = "SELECT * FROM " + identifier(name) + ";";
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
        const int count = sqlite3_column_count(stmt);
        for (int i = 0; i < count; ++i) columns.emplace_back(sqlite3_column_name(stmt, i));
        int result;
        bool ok = true;
        while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
            Row row;
            for (int i = 0; i < count; ++i) {
                auto* copy = sqlite3_value_dup(sqlite3_column_value(stmt, i));
                if (!copy) { ok = false; break; }
                row.emplace_back(copy, sqlite3_value_free);
            }
            if (!ok) break;
            rows.push_back(std::move(row));
        }
        sqlite3_finalize(stmt);
        return ok && result == SQLITE_DONE;
    }
    Row* find(const std::string& key, int id) {
        const int c = column(key);
        if (c < 0) return nullptr;
        // Chi tim mot ID trong snapshot nay, khong can dung chi muc/sap xep.
        std::size_t position;
        return DsaSearch::linearFind(rows, [c, id](const Row& row) {
            return sqlite3_value_int(row[c].get()) == id;
        }, position) ? &rows[position] : nullptr;
    }
};

inline std::string text(const Row& row, int c) {
    const auto* value = sqlite3_value_text(row[c].get());
    return value ? reinterpret_cast<const char*>(value) : "";
}
inline bool isNull(const Row& row, int c) { return sqlite3_value_type(row[c].get()) == SQLITE_NULL; }

// Tao gia tri SQLite co kieu, giu nguyen NULL/BLOB va cac cot database cu.
inline Value value(sqlite3* db, const std::string* textValue) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT ?;", -1, &stmt, nullptr) != SQLITE_OK) return {};
    if (textValue) sqlite3_bind_text(stmt, 1, textValue->c_str(), -1, SQLITE_TRANSIENT);
    else sqlite3_bind_null(stmt, 1);
    Value result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = Value(sqlite3_value_dup(sqlite3_column_value(stmt, 0)), sqlite3_value_free);
    sqlite3_finalize(stmt);
    return result;
}
inline bool set(sqlite3* db, const Table& table, Row& row,
                const std::string& column, const std::string* content) {
    const int c = table.column(column);
    if (c < 0) return false;
    auto replacement = value(db, content);
    if (!replacement) return false;
    row[c] = std::move(replacement);
    return true;
}
inline bool set(sqlite3* db, const Table& table, Row& row,
                const std::string& column, const std::string& content) {
    return set(db, table, row, column, &content);
}
inline bool now(sqlite3* db, std::string& output) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT datetime('now','localtime');", -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    const bool ok = sqlite3_step(stmt) == SQLITE_ROW;
    if (ok) output = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    return ok;
}

inline bool insert(sqlite3* db, const std::string& name, const Table& table,
                   const Row& row, const std::string& updateKey = "") {
    std::string sql = "INSERT INTO " + identifier(name) + " (";
    std::string parameters;
    for (std::size_t i = 0; i < table.columns.size(); ++i) {
        if (i) { sql += ','; parameters += ','; }
        sql += identifier(table.columns[i]); parameters += '?';
    }
    sql += ") VALUES (" + parameters + ")";
    if (!updateKey.empty()) {
        sql += " ON CONFLICT(" + identifier(updateKey) + ") DO UPDATE SET ";
        bool first = true;
        for (const auto& c : table.columns) {
            if (c == updateKey) continue;
            if (!first) sql += ',';
            first = false;
            sql += identifier(c) + "=excluded." + identifier(c);
        }
    }
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    bool ok = row.size() == table.columns.size();
    for (std::size_t i = 0; ok && i < row.size(); ++i)
        ok = sqlite3_bind_value(stmt, static_cast<int>(i + 1), row[i].get()) == SQLITE_OK;
    if (ok) ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

// Xoa trong vector, sau do luu lai cac dong con lai trong cung giao dich.
// Bat buoc hoan tat kiem tra khoa ngoai truoc khi commit.
inline bool erase(sqlite3* db, const std::string& name, const std::string& key, int id) {
    Transaction transaction(db);
    Table table;
    if (!transaction || !table.load(db, name)) return false;
    Row* selected = table.find(key, id);
    if (!selected) return false;
    const std::size_t position = static_cast<std::size_t>(selected - table.rows.data());
    table.rows.erase(table.rows.begin() + position);
    sqlite3_stmt* pragma = nullptr;
    if (sqlite3_prepare_v2(db, "PRAGMA defer_foreign_keys;", -1, &pragma, nullptr) != SQLITE_OK) return false;
    const bool deferred = sqlite3_step(pragma) == SQLITE_ROW && sqlite3_column_int(pragma, 0);
    sqlite3_finalize(pragma);
    struct RestoreForeignKeys {
        sqlite3* db;
        bool restore;
        ~RestoreForeignKeys() { if (restore) exec(db, "PRAGMA defer_foreign_keys=OFF;"); }
    } restore{db, !deferred};
    if (!exec(db, "PRAGMA defer_foreign_keys=ON;") || !exec(db, "DELETE FROM " + identifier(name) + ";"))
        return false;
    for (const auto& row : table.rows) if (!insert(db, name, table, row)) return false;
    if (sqlite3_prepare_v2(db, "PRAGMA foreign_key_check;", -1, &pragma, nullptr) != SQLITE_OK) return false;
    const bool valid = sqlite3_step(pragma) == SQLITE_DONE;
    sqlite3_finalize(pragma);
    if (!valid) return false;
    if (!deferred && !exec(db, "PRAGMA defer_foreign_keys=OFF;")) return false;
    return transaction.commit();
}
}
