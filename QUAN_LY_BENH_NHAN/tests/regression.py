"""Compile all patient tools and exercise temporary databases and check-in hours."""
from contextlib import closing
from datetime import date, timedelta
from pathlib import Path
import sqlite3
import subprocess
import tempfile

MODULE = Path(__file__).resolve().parents[1]


with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    data = root / "QUAN_LY_BENH_NHAN" / "db"
    data.mkdir(parents=True)
    programs = {
        "schema": list(MODULE.joinpath("src/TAO_BANG_DB").glob("*.cpp")),
        "import": list(MODULE.joinpath("src/NHAP_BENH_NHAN").glob("*.cpp")),
        "edit": list(MODULE.joinpath("src/SUA_BENH_NHAN").glob("*.cpp")),
        "delete": list(MODULE.joinpath("src/XOA_BENH_NHAN").glob("*.cpp")),
        "checkin": list(MODULE.joinpath("src/CHECK_IN").glob("*.cpp")),
    }
    for name, files in programs.items():
        subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Wpedantic",
                        *map(str, files), "-lsqlite3", "-o", str(root / (name + ".exe"))], check=True)

    def run(name, text="", ok=True):
        result = subprocess.run([str(root / (name + ".exe"))], cwd=root,
                                input=text, text=True, capture_output=True, timeout=15)
        assert (result.returncode == 0) == ok, result.stdout + result.stderr
        return result.stdout

    # Test the real admission function at boundary times with a controlled clock.
    harness = root / "hours.cpp"
    harness.write_text(r'''
#include <iostream>
#include <string>
#include <ctime>
#include <limits>
#include <cassert>
#include <vector>
static std::time_t testNow;
static std::time_t testTime(std::time_t* output) {
    if (output) *output = testNow;
    return testNow;
}
#define time testTime
#include "khoa.cpp"
#undef time
#include "patient_validation.h"
int main(int argc, char** argv) {
    assert(argc == 2);
    std::string reason;
    for (int minute : {0,449,450,599,600,779,780,899,900,1439}) {
        std::tm clock{};
        clock.tm_year = 126; clock.tm_mon = 9; clock.tm_mday = 5;
        clock.tm_hour = minute / 60; clock.tm_min = minute % 60;
        clock.tm_isdst = -1;
        testNow = std::mktime(&clock);
        const bool admitted = (minute >= 450 && minute < 600) ||
                              (minute >= 780 && minute < 900);
        for (int priority = 1; priority <= 5; ++priority) {
            assert(khoaDangHoatDong("Khoa Noi", priority, reason) == admitted);
            assert(khoaDangHoatDong("Khoa Cap cuu", priority, reason));
        }
    }
    assert(ageFromBirthDate("2000-02-29") >= 0);
    for (std::string invalid : std::vector<std::string>{"2025-02-29", "2000-13-01", "2000-00-01",
                               "2000-04-31", "0000-01-01", std::string(argv[1])}) {
        bool rejected = false;
        try { ageFromBirthDate(invalid); }
        catch (const std::exception&) { rejected = true; }
        assert(rejected);
    }
}
''', encoding="utf-8")
    subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Wpedantic",
                    "-I", str(MODULE / "src/CHECK_IN"), "-I", str(MODULE / "src"),
                    str(harness), "-o", str(root / "hours.exe")], check=True)
    # Tomorrow catches future births even when age would otherwise be zero.
    subprocess.run([str(root / "hours.exe"), (date.today() + timedelta(days=1)).isoformat()],
                   check=True, capture_output=True, text=True)

    run("schema")
    run("schema")
    hospital = data / "hospital.db"
    with closing(sqlite3.connect(hospital)) as db, db:
        db.execute("INSERT INTO patients(id,name,birth_date,age) VALUES (1,'Patient 1','2000-01-01',26)")
        db.execute("INSERT INTO patients(id,name,birth_date,age) VALUES (2,'Patient 2','2000-01-01',26)")
    run("checkin", "1\n1\n1\n4\n0\n")
    run("checkin", "1\n1\n0\n")
    with closing(sqlite3.connect(hospital)) as db:
        assert db.execute("SELECT count(*) FROM checkins").fetchone() == (1,)
        original_id = db.execute("SELECT checkin_id FROM checkins").fetchone()[0]
    run("delete", "1\n", ok=False)
    run("checkin", "3\ny\n1\n2\n1\n4\n0\n")
    with closing(sqlite3.connect(hospital)) as db:
        assert db.execute("SELECT checkin_id FROM checkins").fetchone()[0] > original_id
    run("delete", "1\n")
    run("edit", "2\nChanged but incomplete\n", ok=False)
    with closing(sqlite3.connect(hospital)) as db:
        assert db.execute("SELECT name FROM patients WHERE id=2").fetchone() == ("Patient 2",)
    run("edit", "2\nUpdated\n\n\n\n\n\n")
    with closing(sqlite3.connect(hospital)) as db, db:
        assert db.execute("SELECT name FROM patients WHERE id=2").fetchone() == ("Updated",)
        db.execute("CREATE TRIGGER reject_edit BEFORE UPDATE ON patients BEGIN SELECT RAISE(ABORT,'blocked'); END")
    run("edit", "2\nRejected\n\n\n\n\n\n", ok=False)
    with closing(sqlite3.connect(hospital)) as db, db:
        db.execute("DROP TRIGGER reject_edit")
    csv = data / "benh_nhan_20000.csv"
    header = "name,birth_date,age,gender,hometown,address,phone\n"
    csv.write_text(header + 'CSV Patient,2000-01-01,26,Nam,Hanoi,"Street, City",0123\n', encoding="utf-8")
    run("import")
    run("import")
    with closing(sqlite3.connect(hospital)) as db:
        assert db.execute("SELECT count(*) FROM patients WHERE name='CSV Patient'").fetchone() == (1,)
    csv.write_text(header + "Rollback Patient,2000-01-01,26,Nam,Hanoi,Street,0124\n"
                   + "Future Patient,2999-01-01,0,Nam,Hanoi,Street,0125\n", encoding="utf-8")
    run("import", ok=False)
    with closing(sqlite3.connect(hospital)) as db:
        assert db.execute("SELECT count(*) FROM patients WHERE name='Rollback Patient'").fetchone() == (0,)

print("QUAN_LY_BENH_NHAN: all builds, admission boundaries, CRUD, import, and ID regressions passed")
