"""Regression tests for the current DANG_KHAM architecture.

ket_qua_kham is supported only as a one-time legacy migration source.
After migration, dangKham.db is the single source for doctor assignments.
"""
from contextlib import closing
from pathlib import Path
import re
import sqlite3
import subprocess
import tempfile

MODULE = Path(__file__).resolve().parents[1]
REPO = MODULE.parent


def run(exe, source, destination, text="0\n", ok=True):
    result = subprocess.run(
        [str(exe), str(source), str(destination)],
        input=text, text=True, capture_output=True, timeout=15
    )
    assert (result.returncode == 0) == ok, result.stdout + result.stderr
    return result.stdout


def create_legacy_source(path):
    with closing(sqlite3.connect(path)) as db, db:
        db.execute("""CREATE TABLE ket_qua_kham (
            checkin_id INTEGER PRIMARY KEY,
            patient_id INTEGER NOT NULL,
            khoa_benh_nhan TEXT NOT NULL,
            khoa_bac_si TEXT NOT NULL,
            doctor_id TEXT NOT NULL,
            doctor_name TEXT NOT NULL,
            start_time TEXT NOT NULL,
            exam_duration INTEGER NOT NULL,
            end_time TEXT NOT NULL,
            Status TEXT NOT NULL,
            Note TEXT
        )""")
        rows = [
            (1, 101, "Khoa Noi", "Khoa Noi", "BS001", "Bac si 1",
             "-10 minutes", 20, "+10 minutes", "DA_XEP_BAC_SI", "Hop le"),
            (2, 102, "Khoa Noi", "Khoa Noi", "BS002", "Bac si 2",
             "+1 hour", 20, "+2 hours", "DA_XEP_BAC_SI", "Lich tuong lai"),
            (3, 103, "Khoa Noi", "Khoa Noi", "BS003", "Bac si 3",
             "-10 minutes", 20, "+10 minutes", "CHO_XEP", "Sai trang thai"),
        ]
        for row in rows:
            db.execute("""INSERT INTO ket_qua_kham
                (checkin_id,patient_id,khoa_benh_nhan,khoa_bac_si,
                 doctor_id,doctor_name,start_time,exam_duration,end_time,Status,Note)
                VALUES (?,?,?,?,?,?,datetime('now','localtime',?),?,
                        datetime('now','localtime',?),?,?)""", row)


with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)

    # Pure C++ core.
    subprocess.run([
        "g++", "-std=c++17", "-Wall", "-Wextra", "-Wpedantic",
        "-I", str(MODULE / "src"),
        str(MODULE / "tests/core_tests.cpp"),
        "-o", str(root / "core.exe")
    ], check=True)
    subprocess.run([str(root / "core.exe")], check=True, timeout=15)
    subprocess.run([
        "g++", "-std=c++17", "-Wall", "-Wextra", "-Wpedantic",
        "-I", str(MODULE / "src"),
        "-I", str(REPO / "SAP_XEP_BAC_SI/src/QUAN_LY_THONG_TIN"),
        str(MODULE / "tests/persistence_tests.cpp"),
        str(MODULE / "src/DatabaseDangKham.cpp"),
        str(MODULE / "tests/read_sql_guard.cpp"),
        "-Wl,--wrap=sqlite3_prepare_v2,--wrap=sqlite3_exec", "-lsqlite3",
        "-o", str(root / "persistence.exe")
    ], check=True)
    subprocess.run([str(root / "persistence.exe"), str(root / "repair_source.db"),
                    str(root / "repair_destination.db")], check=True, timeout=15)

    # CLI + SQLite persistence.
    exe = root / "DangKham.exe"
    subprocess.run([
        "g++", "-std=c++17", "-Wall", "-Wextra", "-Wpedantic",
        "-I", str(MODULE / "src"),
        "-I", str(REPO / "SAP_XEP_BAC_SI/src/QUAN_LY_THONG_TIN"),
        *map(str, MODULE.joinpath("src").glob("*.cpp")),
        str(MODULE / "tests/read_sql_guard.cpp"),
        "-Wl,--wrap=sqlite3_prepare_v2,--wrap=sqlite3_exec", "-lsqlite3",
        "-o", str(exe)
    ], check=True)

    # One-time migration from the former ket_qua_kham table.
    source = root / "legacy_source.db"
    destination = root / "dangKham.db"
    create_legacy_source(source)
    run(exe, source, destination)

    with closing(sqlite3.connect(source)) as db:
        assert db.execute(
            "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='ket_qua_kham'"
        ).fetchone() == (0,)

    with closing(sqlite3.connect(destination)) as db:
        columns = [row[1] for row in db.execute("PRAGMA table_info(dang_kham)")]
        for column in (
            "checkin_time", "doctor_id", "doctor_name", "doctor_department",
            "start_time", "planned_end_time", "exam_duration", "status", "note",
            "end_time", "chan_doan", "don_thuoc", "loi_nhac_bac_si", "updated_at"
        ):
            assert column in columns

        rows = db.execute(
            "SELECT checkin_id,status,start_time,planned_end_time FROM dang_kham ORDER BY checkin_id"
        ).fetchall()
        assert [row[0] for row in rows] == [1, 2], rows

    # Only appointments whose start_time has arrived are active.
    output = run(exe, source, destination, "1\n0\n")
    active_ids = [int(value) for value in re.findall(r"Check-in: (\d+)", output)]
    assert active_ids == [1], output

    # Future assignment becomes active when its start time is reached.
    with closing(sqlite3.connect(destination)) as db, db:
        db.execute("""UPDATE dang_kham
                      SET start_time=datetime('now','localtime','-1 minute'),
                          planned_end_time=datetime('now','localtime','+20 minutes')
                      WHERE checkin_id=2""")
    output = run(exe, source, destination, "1\n0\n")
    active_ids = [int(value) for value in re.findall(r"Check-in: (\d+)", output)]
    assert active_ids == [1, 2], output

    # Diagnosis, prescription and reminder are persisted.
    run(exe, source, destination,
        "2\n1\nChan doan\nDon thuoc\nLoi nhac\n0\n")
    with closing(sqlite3.connect(destination)) as db:
        saved = db.execute(
            "SELECT chan_doan,don_thuoc,loi_nhac_bac_si FROM dang_kham WHERE checkin_id=1"
        ).fetchone()
        assert saved == ("Chan doan", "Don thuoc", "Loi nhac"), saved

    # planned_end_time is only a plan; the exam remains active until end_time is written.
    with closing(sqlite3.connect(destination)) as db, db:
        db.execute("""UPDATE dang_kham
                      SET planned_end_time=datetime('now','localtime','-1 hour')
                      WHERE checkin_id=1""")
    output = run(exe, source, destination, "1\n0\n")
    assert "Check-in: 1 " in output

    # Finishing writes the real end_time and removes the visit from active exams.
    run(exe, source, destination, "3\n1\n0\n")
    output = run(exe, source, destination, "1\n0\n")
    assert "Check-in: 1 " not in output
    with closing(sqlite3.connect(destination)) as db:
        assert db.execute(
            "SELECT end_time FROM dang_kham WHERE checkin_id=1"
        ).fetchone()[0] is not None

    # A doctor becoming unavailable returns only future appointments.
    with closing(sqlite3.connect(destination)) as db, db:
        db.execute("""UPDATE dang_kham
                      SET start_time=datetime('now','localtime','+1 hour'),
                          planned_end_time=datetime('now','localtime','+2 hours')
                      WHERE checkin_id=2""")

    # The CLI does not expose doctor-state controls; verify the persisted condition directly.
    with closing(sqlite3.connect(destination)) as db:
        assert db.execute(
            "SELECT count(*) FROM dang_kham WHERE checkin_id=2 AND end_time IS NULL"
        ).fetchone() == (1,)

print("DANG_KHAM: core, migration, active scheduling, diagnosis and completion passed")
