#include "../../include/SqliteMemoryTable.h"
#include <cassert>

int main() {
    sqlite3* db = nullptr;
    assert(sqlite3_open(":memory:", &db) == SQLITE_OK);
    assert(MemoryTable::exec(db,
        "PRAGMA foreign_keys=ON;"
        "CREATE TABLE patients(id INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT,extra BLOB);"
        "CREATE TABLE checkins(id INTEGER PRIMARY KEY,patient_id INTEGER REFERENCES patients(id) ON DELETE RESTRICT);"
        "INSERT INTO patients VALUES(1,'first',NULL),(2,'second',X'00FF'),(3,'third',NULL);"
        "INSERT INTO checkins VALUES(100,2);"));
    // Deleting one patient must preserve other patients and their check-ins.
    assert(MemoryTable::erase(db, "patients", "id", 1));
    MemoryTable::Table patients, checkins;
    assert(patients.load(db, "patients") && patients.rows.size() == 2);
    auto* second = patients.find("id", 2);
    assert(second && sqlite3_value_type((*second)[2].get()) == SQLITE_BLOB);
    assert(sqlite3_value_bytes((*second)[2].get()) == 2);
    assert(checkins.load(db, "checkins") && checkins.rows.size() == 1);
    // Deleting the referenced patient must roll back the entire write.
    assert(!MemoryTable::erase(db, "patients", "id", 2));
    assert(patients.load(db, "patients") && patients.rows.size() == 2);
    assert(!MemoryTable::erase(db, "patients", "id", 999));
    assert(MemoryTable::exec(db,
        "CREATE TRIGGER reject_restore BEFORE INSERT ON patients BEGIN SELECT RAISE(ABORT,'blocked'); END;"));
    assert(!MemoryTable::erase(db, "patients", "id", 3));
    assert(patients.load(db, "patients") && patients.rows.size() == 2);
    assert(MemoryTable::exec(db, "DROP TRIGGER reject_restore;"));
    // UPSERT preserves extra columns and NULL and invokes UPDATE triggers.
    {
        MemoryTable::Transaction transaction(db);
        assert(transaction && patients.load(db, "patients"));
        auto* row = patients.find("id", 3);
        assert(row && MemoryTable::set(db, patients, *row, "name", std::string("changed")));
        assert(MemoryTable::insert(db, "patients", patients, *row, "id") && transaction.commit());
    }
    assert(patients.load(db, "patients"));
    auto* third = patients.find("id", 3);
    assert(third && MemoryTable::text(*third, 1) == "changed" && MemoryTable::isNull(*third, 2));
    // A savepoint failure must not roll back the caller's unrelated writes.
    {
        MemoryTable::Transaction outer(db);
        assert(outer && MemoryTable::exec(db, "INSERT INTO patients VALUES(4,'fourth',NULL);"));
        assert(!MemoryTable::erase(db, "patients", "id", 2));
        assert(outer.commit());
    }
    assert(patients.load(db, "patients") && patients.rows.size() == 3);
    assert(MemoryTable::exec(db, "INSERT INTO patients(name) VALUES('next');"));
    assert(patients.load(db, "patients") && patients.find("id", 5));
    sqlite3_close(db);
}
