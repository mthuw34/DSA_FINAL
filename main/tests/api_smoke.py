"""HTTP integration test on isolated SQLite databases; build the server first."""
import concurrent.futures
from contextlib import closing
from datetime import datetime
import json
import os
import base64
from pathlib import Path
import shutil
import socket
import sqlite3
import subprocess
import tempfile
import time
from urllib.error import HTTPError
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parents[2]
EXE = Path(os.environ.get('HOSPITAL_TEST_EXE', ROOT / 'main/build/hospital_web.exe'))


def main():
    assert EXE.exists(), 'Run main/build.ps1 first'
    with tempfile.TemporaryDirectory(prefix='api-test-', dir=EXE.parent) as folder:
        sandbox = Path(folder)
        (sandbox / 'SAP_XEP_BAC_SI/src').mkdir(parents=True)
        (sandbox / 'SAP_XEP_BAC_SI/db').mkdir()
        shutil.copytree(ROOT / 'main/web', sandbox / 'main/web')
        shutil.copyfile(ROOT / 'SAP_XEP_BAC_SI/db/bac_si_500_chia_khoa.csv',
                        sandbox / 'SAP_XEP_BAC_SI/db/bac_si_500_chia_khoa.csv')
        # Old schema without measurement columns must be migrated at startup.
        (sandbox / 'QUAN_LY_BENH_NHAN/db').mkdir(parents=True)
        with closing(sqlite3.connect(sandbox / 'QUAN_LY_BENH_NHAN/db/hospital.db')) as db:
            db.execute('CREATE TABLE patients (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, '
                       'birth_date TEXT NOT NULL, age INTEGER NOT NULL, gender TEXT, hometown TEXT, address TEXT, phone TEXT)')
        (sandbox / 'TRUY_XUAT_BENH_NHAN/db').mkdir(parents=True)
        with closing(sqlite3.connect(sandbox / 'TRUY_XUAT_BENH_NHAN/db/truyXuat.db')) as db:
            db.execute('CREATE TABLE doctor_state (doctor_id TEXT PRIMARY KEY, duty_mode TEXT NOT NULL, '
                       'busy_until TEXT, busy_reason TEXT NOT NULL)')
            db.execute("INSERT INTO doctor_state VALUES ('legacy-doctor', 'off_duty', NULL, '')")
            db.commit()
        with socket.socket() as sock:
            sock.bind(('127.0.0.1', 0))
            port = sock.getsockname()[1]
        log = open(sandbox / 'server.log', 'w', encoding='utf-8')

        def start(password=None, environment_port=False):
            environment = os.environ.copy()
            for key in ('HOSPITAL_PASSWORD', 'HOSPITAL_BIND', 'PORT'):
                environment.pop(key, None)
            if password is not None:
                environment['HOSPITAL_PASSWORD'] = password
            arguments = [str(EXE), str(sandbox)]
            if environment_port:
                environment['PORT'] = str(port)
            else:
                arguments.append(str(port))
            return subprocess.Popen(arguments, stdout=log, stderr=log, env=environment)

        process = start()

        def request(path, method='GET', data=None, expected=200, raw=None):
            payload = raw if raw is not None else (json.dumps(data).encode() if data is not None else None)
            req = Request(f'http://127.0.0.1:{port}{path}', data=payload, method=method,
                          headers={'Content-Type': 'application/json'})
            try:
                with urlopen(req, timeout=20) as response:
                    status, result = response.status, json.load(response)
            except HTTPError as error:
                status, result = error.code, json.load(error)
            assert status == expected, (path, status, result)
            assert result['ok'] == (status < 400), result
            return result.get('data')

        def ready():
            for _ in range(150):
                if process.poll() is not None:
                    raise AssertionError((sandbox / 'server.log').read_text())
                try:
                    request('/api/health')
                    return
                except OSError:
                    time.sleep(.1)
            raise AssertionError('Server did not start')

        try:
            ready()
            with closing(sqlite3.connect(sandbox / 'DANG_KHAM/db/dangKham.db')) as db:
                assert db.execute("SELECT duty_mode FROM doctor_state WHERE doctor_id='legacy-doctor'").fetchone() == ('off_duty',)
            with closing(sqlite3.connect(sandbox / 'TRUY_XUAT_BENH_NHAN/db/truyXuat.db')) as db:
                assert db.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='doctor_state'").fetchone() is None
            for path, content_type, marker in [('/', 'text/html', 'KMIN HEALTH'),
                    ('/assets/app.css', 'text/css', '.sidebar'),
                    ('/assets/app.js', 'text/javascript', 'loadData')]:
                with urlopen(f'http://127.0.0.1:{port}{path}', timeout=10) as response:
                    assert response.status == 200
                    assert content_type in response.headers['Content-Type']
                    assert marker in response.read().decode('utf-8')
            assert request('/api/patients') == []
            assert len(request('/api/departments')) == 10
            doctors=request('/api/doctors')
            assert len(doctors) == 500
            emergency_coverage = set()
            for doctor in doctors:
                assert doctor['shift_rule'] in ('three_8h_rotating_days_off','weekday_split')
                assert len(doctor['shift_period_start']) == 10
                assert all(s['start_time'] < s['end_time'] and s['date'] == s['start_time'][:10] for s in doctor['shifts'])
                previous_end = None
                for shift in doctor['shifts']:
                    shift_start = datetime.fromisoformat(shift['start_time'])
                    shift_end = datetime.fromisoformat(shift['end_time'])
                    duration = (shift_end - shift_start).total_seconds()
                    assert 0 < duration <= 12 * 3600
                    assert previous_end is None or previous_end <= shift_start
                    previous_end = shift_end
                    if doctor['shift_rule'] == 'three_8h_rotating_days_off':
                        assert duration == 8 * 3600
                        emergency_coverage.add((shift['date'], shift_start.hour))
                    else:
                        assert shift_start.weekday() < 5
                if doctor['shift_rule'] == 'weekday_split':
                    assert len(doctor['shifts']) == 10
                else:
                    starts = [datetime.fromisoformat(s['start_time']) for s in doctor['shifts']]
                    assert all((b - a).days == 2 for a, b in zip(starts, starts[1:]))
                assert sum(s['is_current'] for s in doctor['shifts']) <= 1
                assert doctor['on_duty'] == any(s['is_current'] for s in doctor['shifts'])
            assert len(emergency_coverage) == 7 * 3
            request('/api/patients', 'POST', raw=b'{', expected=400)
            request('/api/patients', 'POST', [], expected=400)
            request('/api/patients', 'POST', {'name':'Test', 'birth_date':'2025-02-30'}, expected=400)
            request('/api/patients/999', expected=404)
            patient = request('/api/patients', 'POST', {
                'name': 'Nguyễn Test', 'birth_date': '2000-01-15', 'height':170, 'weight':65
            }, 201)
            assert abs(patient['bmi'] - 65 / 1.7**2) < .001
            pid = patient['id']
            with closing(sqlite3.connect(sandbox / 'QUAN_LY_BENH_NHAN/db/hospital.db')) as db:
                db.execute('UPDATE patients SET gender=NULL WHERE id=?', (pid,))
                db.commit()
            request(f'/api/patients/{pid}', 'PATCH', {'weight':70})
            assert request(f'/api/patients/{pid}')['weight'] == 70
            with closing(sqlite3.connect(sandbox / 'QUAN_LY_BENH_NHAN/db/hospital.db')) as db:
                assert db.execute('SELECT gender FROM patients').fetchone()[0] is None
            assert len(request('/api/patients?q=Test')) == 1
            request('/api/checkins', 'POST', {'patient_id':pid, 'department':'Unknown', 'priority':1}, 400)
            request('/api/checkins', 'POST', {'patient_id':pid, 'department':'Khoa Cap cuu', 'priority':6}, 400)
            request('/api/checkins', 'POST', {'patient_id':2**64-1, 'department':'Khoa Cap cuu', 'priority':1}, 400)
            ticket = request('/api/checkins', 'POST', {'patient_id':pid, 'department':'Khoa Cap cuu', 'priority':1}, 201)
            cid = ticket['checkin_id']
            request('/api/checkins', 'POST', {'patient_id':pid, 'department':'Khoa Cap cuu', 'priority':1}, 409)
            request(f'/api/patients/{pid}', 'DELETE', expected=409)
            with closing(sqlite3.connect(sandbox / 'THAY_DOI_MUC_DO_UU_TIEN/db/priority.db')) as db:
                checkin_time, last_update = db.execute(
                    'SELECT checkin_time,last_update FROM priority_checkins WHERE checkin_id=?',(cid,)).fetchone()
                assert last_update == checkin_time
            # Check-in appears in queue immediately without an explicit sync.
            assert request('/api/queue')[0]['checkin_id'] == cid
            request(f'/api/checkins/{cid}/priority', 'PATCH', {'priority':2})
            request(f'/api/checkins/{cid}/priority', 'PATCH', {'priority':2})
            assert request('/api/queue')[0]['current_priority'] == 2
            with closing(sqlite3.connect(sandbox / 'THAY_DOI_MUC_DO_UU_TIEN/db/priority.db')) as db:
                manual_update = db.execute(
                    'SELECT last_update FROM priority_checkins WHERE checkin_id=?',(cid,)).fetchone()[0]
                assert manual_update >= checkin_time, (checkin_time, manual_update)
                db.execute("UPDATE priority_checkins SET last_update='2000-01-03 08:00:00' WHERE checkin_id=?", (cid,))
                db.commit()
            deadline = time.monotonic() + 12
            while request('/api/queue')[0]['current_priority'] != 1:
                assert time.monotonic() < deadline, 'Background sync did not increase priority'
                time.sleep(.2)
            scheduled = request('/api/assignments', 'POST', {})
            assert scheduled['assigned_count'] == 1, scheduled
            original = request('/api/assignments')
            assert request('/api/assignments', 'POST', {})['assigned_count'] == 0
            assert request('/api/assignments') == original
            assert request('/api/queue') == []
            request(f'/api/checkins/{cid}/priority', 'PATCH', {'priority':3}, 409)
            # Restart must preserve appointments and doctor availability.
            process.terminate()
            process.wait(timeout=10)
            process = start()
            ready()
            assert request('/api/assignments', 'POST', {})['assigned_count'] == 0
            assert request('/api/assignments') == original
            # Make the saved appointment currently active regardless of fixture doctor shifts.
            db = sqlite3.connect(sandbox / 'DANG_KHAM/db/dangKham.db')
            db.execute("UPDATE dang_kham SET start_time=datetime('now','localtime','-1 minute'), planned_end_time=datetime('now','localtime','+20 minutes')")
            db.commit()
            db.close()
            request('/api/exams/sync', 'POST')
            assert request('/api/exams')[0]['checkin_id'] == cid
            request(f'/api/exams/{cid}/diagnosis', 'PATCH', {'diagnosis':'Test diagnosis', 'prescription':'Test', 'reminder':'Again'})
            request('/api/exams/sync', 'POST')
            assert request('/api/exams')[0]['diagnosis'] == 'Test diagnosis'
            request(f'/api/exams/{cid}/finish', 'POST')
            request('/api/exams/sync', 'POST')
            assert request('/api/exams') == []
            assert request('/api/exams?active=false')[0]['end_time'] is not None
            request(f'/api/exams/{cid}/diagnosis', 'PATCH', {'diagnosis':'Too late'}, 409)
            request(f'/api/exams/{cid}/finish', 'POST', expected=409)
            # Two simultaneous check-ins: only one may succeed.
            new = request('/api/patients', 'POST', {'name':'Race', 'birth_date':'2001-01-01'}, 201)
            def race(_):
                req = Request(f'http://127.0.0.1:{port}/api/checkins', method='POST',
                              data=json.dumps({'patient_id':new['id'], 'department':'Khoa Cap cuu', 'priority':1}).encode(),
                              headers={'Content-Type':'application/json'})
                try:
                    with urlopen(req, timeout=20) as response:
                        return response.status
                except HTTPError as error:
                    return error.code
            with concurrent.futures.ThreadPoolExecutor(2) as pool:
                assert sorted(pool.map(race, range(2))) == [201,409]
            assert request('/api/assignments', 'POST', {})['assigned_count'] == 1
            booked = request('/api/assignments')
            assert len(booked) == 2
            assert booked[0]['checkin_id'] != booked[1]['checkin_id']
            # Real doctor availability, chosen-doctor scheduling and pending cancellation.
            did = next(d['id'] for d in request('/api/doctors')
                       if d['department'] == 'Khoa Cap cuu' and d['id'] not in {a['doctor_id'] for a in booked})
            endpoint = f'/api/doctors/{did}/status'
            request('/api/doctors/unknown/status', 'PATCH', {'busy_minutes':30,'busy_reason':'Meeting'}, 404)
            request(endpoint, 'PATCH', {'duty_mode':'invalid'}, 400)
            request(endpoint, 'PATCH', {'busy_minutes':-1}, 400)
            request(endpoint, 'PATCH', {'busy_minutes':30,'busy_reason':' '}, 400)
            request(endpoint, 'PATCH', {'duty_mode':'on_duty'})
            request('/api/assignments', 'POST', {'doctor_id':did,'department':'Khoa Noi'}, 400)
            request('/api/assignments', 'POST', {'doctor_id':'unknown'}, 404)
            request('/api/assignments', 'POST', {'doctor_id':''}, 400)
            test_ids=[]
            for i in range(2):
                person=request('/api/patients','POST',{'name':f'Doctor test {i}','birth_date':'2000-01-01'},201)
                test_ids.append(request('/api/checkins','POST',{'patient_id':person['id'],'department':'Khoa Cap cuu','priority':1},201)['checkin_id'])
            result=request('/api/assignments','POST',{'doctor_id':did})
            selected=[a for a in result['assignments'] if a['checkin_id'] in test_ids]
            assert len(selected)==1 and selected[0]['doctor_id']==did
            first_called=selected[0]['checkin_id']
            remaining=next(checkin_id for checkin_id in test_ids if checkin_id != first_called)
            assert any(q['checkin_id']==remaining for q in request('/api/queue'))
            request('/api/queue/sync','POST')
            assert all(q['checkin_id'] != first_called for q in request('/api/queue'))
            with closing(sqlite3.connect(sandbox / 'THAY_DOI_MUC_DO_UU_TIEN/db/priority.db')) as db:
                assert db.execute('SELECT 1 FROM priority_checkins WHERE checkin_id=?',(first_called,)).fetchone() is None
            with closing(sqlite3.connect(sandbox / 'TRUY_XUAT_BENH_NHAN/db/truyXuat.db')) as db:
                assert sum(db.execute(f'SELECT COUNT(*) FROM {table} WHERE checkin_id=?',(first_called,)).fetchone()[0]
                           for table in ('queue_khoa_cap_cuu','queue_khoa_noi','queue_khoa_ngoai',
                                         'queue_khoa_tim_mach','queue_khoa_nhi','queue_khoa_san',
                                         'queue_khoa_tai_mui_hong','queue_khoa_mat','queue_khoa_da_lieu',
                                         'queue_khoa_than_kinh')) == 0
            active=[e for e in request('/api/exams') if e['doctor_id']==did]
            assert len(active)==1
            request('/api/assignments','POST',{'doctor_id':did},409)
            result=request(endpoint,'PATCH',{'busy_minutes':30,'busy_reason':'Meeting'})
            assert result['returned_to_queue']==0
            assert any(q['checkin_id']==remaining for q in request('/api/queue'))
            assert any(e['checkin_id']==active[0]['checkin_id'] for e in request('/api/exams'))
            doctor=next(d for d in request('/api/doctors') if d['id']==did)
            assert doctor['busy'] and doctor['status']=='examining'
            request(f"/api/exams/{active[0]['checkin_id']}/finish",'POST')
            assert next(d for d in request('/api/doctors') if d['id']==did)['status']=='busy'
            process.terminate()
            process.wait(timeout=10)
            process=start()
            ready()
            assert next(d for d in request('/api/doctors') if d['id']==did)['busy']
            request('/api/assignments','POST',{'doctor_id':did},409)
            request(endpoint,'PATCH',{'busy_minutes':0})
            assert not next(d for d in request('/api/doctors') if d['id']==did)['busy']
            request(endpoint,'PATCH',{'duty_mode':'off_duty'})
            request('/api/assignments','POST',{'doctor_id':did},409)
            request(endpoint,'PATCH',{'duty_mode':'on_duty'})
            # A free doctor takes the next patient on a later call; no future slot is pre-booked.
            result=request('/api/assignments','POST',{'doctor_id':did})
            assert [a['checkin_id'] for a in result['assignments']].count(remaining)==1
            assert not any(q['checkin_id']==remaining for q in request('/api/queue'))
            with closing(sqlite3.connect(sandbox / 'THAY_DOI_MUC_DO_UU_TIEN/db/priority.db')) as db:
                assert db.execute('SELECT 1 FROM priority_checkins WHERE checkin_id=?',(remaining,)).fetchone() is None
            removable = request('/api/patients', 'POST', {'name':'Delete', 'birth_date':'2000-01-01'}, 201)
            request(f"/api/patients/{removable['id']}", 'DELETE')
            request(f"/api/patients/{removable['id']}", expected=404)
            process.terminate()
            process.wait(timeout=10)
            password = 'isolated-test-password'
            process = start(password, environment_port=True)
            ready()
            request('/api/patients', expected=401)
            for path in ('/', '/assets/app.js', '/assets/app.css', '/api/doctors'):
                try:
                    urlopen(f'http://127.0.0.1:{port}{path}', timeout=10)
                    raise AssertionError('Unauthenticated route accepted')
                except HTTPError as error:
                    assert error.code == 401
                    assert 'Basic realm=' in error.headers['WWW-Authenticate']
            credentials = base64.b64encode(f'admin:{password}'.encode()).decode()
            authenticated = Request(f'http://127.0.0.1:{port}/api/patients',
                                    headers={'Authorization':f'Basic {credentials}'})
            with urlopen(authenticated, timeout=10) as response:
                assert response.status == 200 and json.load(response)['ok']
            wrong = Request(f'http://127.0.0.1:{port}/api/patients', headers={'Authorization':'Basic wrong'})
            try:
                urlopen(wrong, timeout=10)
                raise AssertionError('Wrong password accepted')
            except HTTPError as error:
                assert error.code == 401
            process.terminate()
            process.wait(timeout=10)
            for variables in ({'HOSPITAL_BIND':'0.0.0.0'}, {'HOSPITAL_PASSWORD':'too-short'}, {'PORT':'4294985376'}):
                environment = os.environ.copy()
                for key in ('HOSPITAL_PASSWORD', 'HOSPITAL_BIND', 'PORT'):
                    environment.pop(key, None)
                environment.update(variables)
                rejected = subprocess.run([str(EXE),str(sandbox)], env=environment,
                                          capture_output=True, timeout=10)
                assert rejected.returncode == 1, variables
            print('PASS: API validation, CRUD, immediate queue, chosen doctor, busy/duty persistence, pending cancellation, background admission, diagnosis, concurrency, password protection')
        finally:
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=10)
            log.close()


if __name__ == '__main__':
    main()
