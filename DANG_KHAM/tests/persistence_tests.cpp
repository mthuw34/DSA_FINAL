#include "DatabaseDangKham.h"
#include "../../include/SqliteMemoryTable.h"
#include <cassert>

int main(int argc, char** argv) {
    assert(argc == 3);
    DatabaseDangKham database;
    assert(database.mo(argv[1], argv[2]));
    sqlite3* db = database.get();
    assert(MemoryTable::exec(db,
        "CREATE TABLE dang_kham(checkin_id INTEGER PRIMARY KEY,patient_id INTEGER NOT NULL,"
        "department TEXT NOT NULL,checkin_time TEXT NOT NULL,doctor_id TEXT,doctor_name TEXT,"
        "start_time TEXT,chan_doan TEXT,payload BLOB);"
        "INSERT INTO dang_kham VALUES"
        "(1,101,'Noi','2020-01-01 08:00:00','BS1','Doctor','2020-01-01 08:00:00','saved',NULL),"
        "(2,102,'Noi','2020-01-01 08:00:00','BS1','Doctor','2020-01-01 07:00:00',NULL,X'00FF'),"
        "(3,103,'Noi','2020-01-01 08:00:00',NULL,NULL,NULL,NULL,NULL),"
        "(4,104,'Noi','2020-01-01 08:00:00','BS2','Doctor','2999-01-01 08:00:00',NULL,NULL);"));
    assert(database.taoCauTruc());
    MemoryTable::Table archive;
    assert(archive.load(db, "dang_kham_assignment_archive") && archive.rows.size() == 1);
    const auto& archived = archive.rows[0];
    assert(sqlite3_value_int(archived[archive.column("checkin_id")].get()) == 2);
    assert(MemoryTable::text(archived, archive.column("doctor_id")) == "BS1");
    assert(sqlite3_value_type(archived[archive.column("payload")].get()) == SQLITE_BLOB);
    std::vector<BenhNhanKham> assignments;
    assert(database.docPhanBacSi(assignments) && assignments.size() == 2);
    assert(assignments[0].CheckinId == 1 && assignments[1].CheckinId == 4);
    assert(database.luuChanDoan(1, "diagnosis", "prescription", "reminder"));
    assert(!database.luuChanDoan(999, "", "", ""));
    assert(database.ketThucPhien(1));
    assert(!database.ketThucPhien(1) && !database.luuChanDoan(1, "overwrite", "", ""));
    BenhNhanKham replacement = assignments[0];
    replacement.DoctorName = "overwrite";
    assert(database.ghiPhanBacSi({replacement}));
    assert(database.xoaCaChuaBatDauCuaBacSi("")); // NULL doctor_id is not an empty doctor ID.
    assert(database.xoaCaChuaBatDauCuaBacSi("BS2"));
    MemoryTable::Table table;
    assert(table.load(db, "dang_kham") && table.rows.size() == 3);
    const auto* completed = table.find("checkin_id", 1);
    assert(completed && MemoryTable::text(*completed, table.column("doctor_name")) == "Doctor");
    assert(MemoryTable::text(*completed, table.column("chan_doan")) == "diagnosis");
    assert(!table.find("checkin_id", 4));
    assert(database.taoCauTruc());
    assert(archive.load(db, "dang_kham_assignment_archive") && archive.rows.size() == 1);
}
