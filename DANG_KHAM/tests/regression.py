"""Run with Python and g++/SQLite available; databases are temporary fixtures."""
from contextlib import closing
from pathlib import Path
import sqlite3
import re
import subprocess
import tempfile

MODULE = Path(__file__).resolve().parents[1]


def run(exe, source, destination, text="0\n", ok=True):
    result = subprocess.run([str(exe), str(source), str(destination)],
                            input=text, text=True, capture_output=True, timeout=15)
    assert (result.returncode == 0) == ok, result.stdout + result.stderr
    return result.stdout


with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Wpedantic",
                    "-I", str(MODULE / "src"), str(MODULE / "tests/core_tests.cpp"),
                    "-o", str(root / "core.exe")], check=True)
    subprocess.run([str(root / "core.exe")], check=True, timeout=15)
    exe = root / "DangKham.exe"
    subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Wpedantic",
                    *map(str, MODULE.joinpath("src").glob("*.cpp")),
                    str(MODULE / "tests/read_sql_guard.cpp"), "-Wl,--wrap=sqlite3_prepare_v2",
                    "-lsqlite3", "-o", str(exe)], check=True)
    source = root / "source.db"
    with closing(sqlite3.connect(source)) as db, db:
        db.execute("""CREATE TABLE ket_qua_kham (
            checkin_id INTEGER PRIMARY KEY, patient_id INTEGER, khoa_bac_si TEXT,
            doctor_id TEXT, doctor_name TEXT, start_time TEXT, end_time TEXT, Status TEXT)""")
        # Active, expired, future, wrong status, invalid dates, and boundary end.
        for identity, start, end, status in [
            (1, "-10 minutes", "+1 hour", "DA_XEP_BAC_SI"),
            (2, "-2 hours", "-1 hour", "DA_XEP_BAC_SI"),
            (3, "+1 hour", "+2 hours", "DA_XEP_BAC_SI"),
            (4, "-10 minutes", "+1 hour", "CHO_XEP"),
            (5, "invalid", "+1 hour", "DA_XEP_BAC_SI"),
            (6, "-10 minutes", "invalid", "DA_XEP_BAC_SI"),
            (7, "-10 minutes", "+0 seconds", "DA_XEP_BAC_SI"),
        ]:
            db.execute("""INSERT INTO ket_qua_kham VALUES (?, ?, 'Khoa Noi', 'BS1', 'Bac si',
                       datetime('now','localtime',?), datetime('now','localtime',?), ?)""",
                       (identity, identity + 100, start, end, status))

    for schema in ("fresh", "legacy", "without_checkin_time"):
        destination = root / (schema + ".db")
        if schema != "fresh":
            with closing(sqlite3.connect(destination)) as db, db:
                extra = "checkin_time TEXT NOT NULL," if schema == "legacy" else ""
                db.execute(f"""CREATE TABLE dang_kham (
                    checkin_id INTEGER PRIMARY KEY, patient_id INTEGER NOT NULL,
                    department TEXT NOT NULL, {extra} start_time TEXT, end_time TEXT,
                    chan_doan TEXT, don_thuoc TEXT, updated_at TEXT)""")
        run(exe, source, destination)
        with closing(sqlite3.connect(destination)) as db, db:
            rows = db.execute("SELECT checkin_id, checkin_time, start_time, end_time FROM dang_kham").fetchall()
            assert len(rows) == 1 and rows[0][0] == 1 and rows[0][1] == rows[0][2] and rows[0][3] is None, rows
            assert "loi_nhac_bac_si" in [r[1] for r in db.execute("PRAGMA table_info(dang_kham)")]
        output = run(exe, source, destination, "2\n1\nChan doan\nDon thuoc\nLoi nhac\n1\n0\n")
        assert "Chan doan" in output and "Loi nhac" in output
        with closing(sqlite3.connect(destination)) as db, db:
            expected = db.execute("SELECT chan_doan, don_thuoc, loi_nhac_bac_si FROM dang_kham").fetchone()
            assert expected == ("Chan doan", "Don thuoc", "Loi nhac"), expected
            db.execute("UPDATE dang_kham SET checkin_time='2000-01-01 07:30:00'")
        run(exe, source, destination)
        with closing(sqlite3.connect(destination)) as db:
            assert db.execute("SELECT checkin_time FROM dang_kham").fetchone() == ("2000-01-01 07:30:00",)
            assert db.execute("SELECT chan_doan, don_thuoc, loi_nhac_bac_si FROM dang_kham").fetchone() == expected
        run(exe, source, destination, "2\n1\n\nThuoc khong luu\nNhac khong luu\n0\n")
        with closing(sqlite3.connect(destination)) as db, db:
            assert db.execute("SELECT chan_doan, don_thuoc, loi_nhac_bac_si FROM dang_kham").fetchone() == expected
        run(exe, source, destination, "3\n1\n0\n")
        run(exe, source, destination, "2\n1\n0\n")
        with closing(sqlite3.connect(destination)) as db, db:
            assert db.execute("SELECT end_time FROM dang_kham").fetchone()[0] is not None
            assert db.execute("SELECT chan_doan, don_thuoc, loi_nhac_bac_si FROM dang_kham").fetchone() == expected

    # Existing active sessions outlive the scheduled end; clinical data survives sync.
    destination = root / "overrun.db"
    run(exe, source, destination)
    with closing(sqlite3.connect(source)) as db, db:
        db.execute("UPDATE ket_qua_kham SET end_time=datetime('now','localtime','-1 minute') WHERE checkin_id=1")
    run(exe, source, destination)
    with closing(sqlite3.connect(destination)) as db, db:
        assert db.execute("SELECT end_time FROM dang_kham WHERE checkin_id=1").fetchone() == (None,)
    # A fresh destination must not resurrect that expired visit.
    run(exe, source, root / "expired.db")
    with closing(sqlite3.connect(root / "expired.db")) as db, db:
        assert db.execute("SELECT count(*) FROM dang_kham").fetchone() == (0,)
    # Missing source table fails cleanly.
    empty = root / "empty.db"
    sqlite3.connect(empty).close()
    run(exe, empty, root / "missing_table.db", ok=False)

    # New visits become available while the menu stays open.
    process = subprocess.Popen([str(exe), str(source), str(root / "refresh.db")],
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, text=True)
    try:
        prompt = ""
        while not prompt.endswith("Lua chon: "):
            character = process.stdout.read(1)
            assert character, prompt
            prompt += character
        with closing(sqlite3.connect(source)) as db, db:
            db.execute("""UPDATE ket_qua_kham SET start_time=datetime('now','localtime','-1 minute'),
                       end_time=datetime('now','localtime','+1 hour') WHERE checkin_id=3""")
        output, error = process.communicate("1\n0\n", timeout=15)
        assert process.returncode == 0 and "BN: 103" in output, output + error
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()
        for pipe in (process.stdin, process.stdout, process.stderr):
            pipe.close()

    # Stable start-time/check-in ordering, implemented by the C++ core.
    with closing(sqlite3.connect(source)) as db, db:
        for identity, start in [(8, "-40 minutes"), (9, "-50 minutes"), (10, "-40 minutes")]:
            db.execute("""INSERT INTO ket_qua_kham VALUES (?, ?, 'Khoa Noi', 'BS1', 'Bac si',
                       datetime('now','localtime',?), datetime('now','localtime','+1 hour'), 'DA_XEP_BAC_SI')""",
                       (identity, identity + 100, start))
    output = run(exe, source, root / "order.db", "1\n0\n")
    assert [int(x) for x in re.findall(r"Check-in: (\d+)", output)] == [9, 8, 10, 3], output
    run(exe, source, root / "order.db", "3\n9\n0\n")
    output = run(exe, source, root / "order.db", "1\n0\n")
    assert "Check-in: 9 " not in output, output

    # A failure on the second new row must undo the first insert in the same sync.
    destination = root / "atomic.db"
    with closing(sqlite3.connect(destination)) as db, db:
        db.execute("""CREATE TABLE dang_kham (checkin_id INTEGER PRIMARY KEY, patient_id INTEGER NOT NULL,
                   department TEXT NOT NULL, checkin_time TEXT NOT NULL)""")
        db.execute("""CREATE TRIGGER reject_eight BEFORE INSERT ON dang_kham
                   WHEN NEW.checkin_id=8 BEGIN SELECT RAISE(ABORT,'blocked'); END""")
    run(exe, source, destination, ok=False)
    with closing(sqlite3.connect(destination)) as db:
        assert db.execute("SELECT count(*) FROM dang_kham").fetchone() == (0,)

print("DANG_KHAM: compile, DSA core, SQL read guard, ordering, schema, atomic sync, diagnosis, and completion passed")
