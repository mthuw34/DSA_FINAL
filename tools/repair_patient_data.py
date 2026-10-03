"""Back up and repair the repository's patient data without changing referenced IDs."""
import collections
from contextlib import closing
import csv
import datetime
import pathlib
import shutil
import sqlite3

ROOT = pathlib.Path(__file__).resolve().parents[1]


def repair(as_of):
    csv_path = ROOT / 'QUAN_LY_BENH_NHAN/db/benh_nhan_20000.csv'
    db_path = ROOT / 'QUAN_LY_BENH_NHAN/db/hospital.db'
    backup = ROOT / '.local-repair' / datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    backup.mkdir(parents=True)
    shutil.copy2(csv_path, backup / csv_path.name)
    db = sqlite3.connect(db_path)
    with closing(sqlite3.connect(backup / 'hospital.db')) as target:
        db.backup(target)
    db.execute('PRAGMA foreign_keys = ON')

    def age(value):
        born = datetime.date.fromisoformat(value)
        if born > as_of:
            raise ValueError('Future birth date')
        return as_of.year - born.year - ((as_of.month, as_of.day) < (born.month, born.day))

    with csv_path.open(encoding='utf-8-sig', newline='') as source:
        rows = list(csv.reader(source))
    changed = 0
    for row in rows[1:]:
        if len(row) != 7:
            raise ValueError('Invalid CSV row')
        current_age = str(age(row[1]))
        changed += row[2] != current_age
        row[2] = current_age

    # Keep every ID referenced by any current queue, including cached databases.
    referenced = set()
    for path in ROOT.rglob('*.db'):
        if '.local-repair' in path.parts:
            continue
        with closing(sqlite3.connect(path.as_uri() + '?mode=ro', uri=True)) as source:
            for (table,) in source.execute("SELECT name FROM sqlite_master WHERE type='table'"):
                quoted = '"' + table.replace('"', '""') + '"'
                if 'patient_id' in [col[1] for col in source.execute('PRAGMA table_info(' + quoted + ')')]:
                    referenced.update(row[0] for row in source.execute('SELECT patient_id FROM ' + quoted))

    removed = 0
    with db:
        db.execute('BEGIN IMMEDIATE')
        groups = collections.defaultdict(list)
        for row in db.execute('SELECT id, name, birth_date, gender, hometown, address, phone, height, weight, bmi FROM patients ORDER BY id'):
            groups[row[1:7]].append(row)
        for records in groups.values():
            ids = [row[0] for row in records]
            measurements = [{row[column] for row in records if row[column] is not None}
                            for column in (7, 8, 9)]
            # Conflicting clinical values need manual review; never discard them.
            if any(len(values) > 1 for values in measurements):
                continue
            keep = set(ids) & referenced
            if not keep:
                keep.add(ids[0])
            merged = [next(iter(values)) if values else None for values in measurements]
            for patient_id in keep:
                db.execute('UPDATE patients SET height = ?, weight = ?, bmi = ? WHERE id = ?',
                           (*merged, patient_id))
            for patient_id in ids:
                if patient_id not in keep:
                    db.execute('DELETE FROM patients WHERE id = ?', (patient_id,))
                    removed += 1
        updates = [(age(birth), patient_id) for patient_id, birth in db.execute('SELECT id, birth_date FROM patients')]
        db.executemany('UPDATE patients SET age = ? WHERE id = ?', updates)
        assert not db.execute('PRAGMA foreign_key_check').fetchall()
        assert db.execute('PRAGMA integrity_check').fetchone()[0] == 'ok'
    db.close()
    pending = csv_path.with_suffix('.csv.tmp')
    with pending.open('w', encoding='utf-8', newline='') as output:
        csv.writer(output).writerows(rows)
    pending.replace(csv_path)
    print(f'Removed {removed} duplicate records; updated {changed} CSV ages as of {as_of}.')
    print(f'Backup: {backup}')


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--as-of', type=datetime.date.fromisoformat, default=datetime.date.today())
    repair(parser.parse_args().as_of)
