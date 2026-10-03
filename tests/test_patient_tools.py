"""Integration checks against temporary databases; never touches repository data."""
import csv
import datetime
import pathlib
import sqlite3
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
BIN = ROOT / '.local-repair/bin'


class PatientToolsTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.cwd = pathlib.Path(self.temp.name)
        self.data = self.cwd / 'QUAN_LY_BENH_NHAN/db'
        self.data.mkdir(parents=True)
        self.db = sqlite3.connect(self.data / 'hospital.db')
        self.addCleanup(self.db.close)
        self.db.executescript('''
            CREATE TABLE patients(id INTEGER PRIMARY KEY, name TEXT NOT NULL,
                birth_date TEXT, age INTEGER NOT NULL, gender TEXT, hometown TEXT,
                address TEXT, phone TEXT);
            CREATE TABLE checkins(checkin_id INTEGER PRIMARY KEY,
                patient_id INTEGER REFERENCES patients(id));
        ''')
        self.row = ['Test Patient', '2000-01-01', '1', 'Nu', 'Hue', 'Street, City', '0123456789']

    def write_csv(self, rows):
        with (self.data / 'benh_nhan_20000.csv').open('w', encoding='utf-8', newline='') as output:
            writer = csv.writer(output)
            writer.writerow(['name', 'birth_date', 'age', 'gender', 'hometown', 'address', 'phone'])
            writer.writerows(rows)

    def run_tool(self, tool, text=''):
        return subprocess.run([str(BIN / (tool + '.exe'))], input=text, text=True,
                              capture_output=True, cwd=self.cwd, timeout=30)

    def test_import_is_repeatable_and_uses_birth_date(self):
        self.write_csv([self.row, self.row])
        for _ in range(2):
            result = self.run_tool('import')
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(self.db.execute('SELECT count(*), age FROM patients').fetchone(),
                         (1, datetime.date.today().year - 2000))
        self.assertEqual(self.db.execute('SELECT address, phone FROM patients').fetchone(),
                         ('Street, City', '0123456789'))

    def test_invalid_rows_roll_back_import(self):
        for field, value in [(2, '-1'), (2, '20abc'), (1, '2025-02-29'), (1, '2999-01-01')]:
            bad = self.row.copy()
            bad[field] = value
            self.write_csv([self.row, bad])
            self.assertNotEqual(self.run_tool('import').returncode, 0)
            self.assertEqual(self.db.execute('SELECT count(*) FROM patients').fetchone()[0], 0)

    def test_delete_missing_invalid_and_referenced(self):
        result = self.run_tool('delete', '999\n')
        self.assertIn('Khong tim thay', result.stdout)
        self.assertNotEqual(self.run_tool('delete', '1abc\n').returncode, 0)
        self.write_csv([self.row])
        self.assertEqual(self.run_tool('import').returncode, 0)
        self.db.execute('INSERT INTO checkins VALUES (1, 1)')
        self.db.commit()
        self.assertNotEqual(self.run_tool('delete', '1\n').returncode, 0)
        self.assertEqual(self.db.execute('SELECT count(*) FROM patients').fetchone()[0], 1)
        self.db.execute('DELETE FROM checkins')
        self.db.commit()
        self.assertEqual(self.run_tool('delete', '1\n').returncode, 0)
        self.assertEqual(self.db.execute('SELECT count(*) FROM patients').fetchone()[0], 0)

    def test_edit_validates_date_and_recalculates_age(self):
        self.write_csv([self.row])
        self.assertEqual(self.run_tool('import').returncode, 0)
        result = self.run_tool('edit', '1\n\n\n2025-02-29\n\n\n\n')
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.db.execute('SELECT birth_date FROM patients').fetchone()[0], '2000-01-01')
        result = self.run_tool('edit', '1\n\n\n2004-01-01\n\n\n\n')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(self.db.execute('SELECT age FROM patients').fetchone()[0], datetime.date.today().year - 2004)


if __name__ == '__main__':
    unittest.main()
