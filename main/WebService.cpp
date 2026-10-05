#include "WebService.h"
#include <cmath>
#include <filesystem>
#include <memory>
#include <algorithm>
#include "../SAP_XEP_BAC_SI/src/THOI_GIAN/ThoiGian.h"
#include "../QUAN_LY_BENH_NHAN/src/HospitalPersistence.h"
#include "../QUAN_LY_BENH_NHAN/src/patient_validation.h"
#include "../QUAN_LY_BENH_NHAN/src/CHECK_IN/khoa.h"
#include "../THAY_DOI_MUC_DO_UU_TIEN/src/DONG_BO/PrioritySync.h"
#include "../THAY_DOI_MUC_DO_UU_TIEN/src/THAY_DOI_TU_DONG/AutoPriority.h"
#include "../THAY_DOI_MUC_DO_UU_TIEN/src/CAP_NHAT_THU_CONG/PriorityManager.h"
#include "../TRUY_XUAT_BENH_NHAN/src/TRUY_XUAT/TruyXuat.h"
#include "../TRUY_XUAT_BENH_NHAN/src/TAO_BANG/TaoBangTruyXuat.h"
#include "../SAP_XEP_BAC_SI/src/THUAT_TOAN_CHINH/QuanLyKhamBenh.h"

namespace hospital_web {
namespace {
const char* hospitalPath = "QUAN_LY_BENH_NHAN/db/hospital.db";
const char* priorityPath = "THAY_DOI_MUC_DO_UU_TIEN/db/priority.db";
const char* retrievalPath = "TRUY_XUAT_BENH_NHAN/db/truyXuat.db";
const char* doctorsPath = "SAP_XEP_BAC_SI/db/bac_si_500_chia_khoa.csv";
void require(bool condition, int status, const std::string& message) {
    if (!condition) throw ApiError(status, message);
}
void execute(sqlite3* db, const char* sql) {
    require(sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK, 500, "Loi ghi database");
}
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
Statement prepare(sqlite3* db, const char* sql) {
    sqlite3_stmt* stmt = nullptr;
    const int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    Statement owned(stmt, sqlite3_finalize);
    require(result == SQLITE_OK, 500, "Loi chuan bi database");
    return owned;
}
void bindText(sqlite3_stmt* stmt, int index, const std::string& value) {
    require(sqlite3_bind_text(stmt, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT)
        == SQLITE_OK, 500, "Loi du lieu database");
}
struct Transaction {
    sqlite3* db;
    bool committed = false;
    explicit Transaction(sqlite3* database) : db(database) { execute(db, "BEGIN IMMEDIATE;"); }
    ~Transaction() { if (!committed) sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr); }
    void commit() { execute(db, "COMMIT;"); committed = true; }
};
void open(sqlite3*& db, const char* path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    require(sqlite3_open(path, &db) == SQLITE_OK, 500, "Khong mo duoc database");
    sqlite3_busy_timeout(db, 5000);
    execute(db, "PRAGMA foreign_keys=ON;");
}
void migrate(sqlite3* db, const char* column) {
    auto stmt = prepare(db, "PRAGMA table_info(patients);");
    bool found = false;
    int result;
    while ((result = sqlite3_step(stmt.get())) == SQLITE_ROW)
        if (HospitalPersistence::text(stmt.get(), 1) == column) found = true;
    require(result == SQLITE_DONE, 500, "Khong doc duoc schema");
    stmt.reset();
    if (!found) execute(db, (std::string("ALTER TABLE patients ADD COLUMN ") + column + " REAL;").c_str());
}
std::string stringField(const Json& data, const char* key, bool required = false) {
    if (!data.contains(key)) { require(!required, 400, std::string("Thieu truong ") + key); return ""; }
    require(data[key].is_string(), 400, std::string(key) + " phai la chuoi");
    auto value = data[key].get<std::string>();
    require(value.size() <= 10000 && value.find('\0') == std::string::npos, 400, "Chuoi khong hop le");
    require(!required || value.find_first_not_of(" \t\r\n") != std::string::npos, 400, std::string(key) + " khong duoc rong");
    return value;
}
int intField(const Json& data, const char* key, int low, int high) {
    require(data.contains(key) && data[key].is_number_integer(), 400, std::string(key) + " phai la so nguyen");
    // So sánh trước khi thu hẹp để số JSON lớn không bị tràn int.
    require(data[key] >= low && data[key] <= high, 400, std::string(key) + " ngoai pham vi");
    return data[key].get<int>();
}
Json patientJson(const Patient& p) {
    return {{"id",p.id},{"name",p.name},{"birth_date",p.birthDate},{"age",p.age},
        {"phone",p.phone},{"gender",p.gender},{"hometown",p.hometown},{"address",p.address},
        {"height",p.height},{"weight",p.weight},{"bmi",p.bmi}};
}
Json checkInJson(const CheckInRecord& r) {
    return {{"checkin_id",r.id},{"patient_id",r.patientId},{"department",r.department},
        {"priority",r.priority},{"checkin_time",r.time}};
}
Json optionalJson(const std::optional<std::string>& value) { return value ? Json(*value) : Json(nullptr); }
Json examJson(const ExamSession& e) {
    return {{"checkin_id",e.checkinId},{"patient_id",e.patientId},{"department",e.department},
        {"doctor_id",optionalJson(e.doctorId)},{"doctor_name",optionalJson(e.doctorName)},
        {"checkin_time",optionalJson(e.checkinTime)},{"start_time",optionalJson(e.startTime)},
        {"end_time",optionalJson(e.endTime)},{"diagnosis",optionalJson(e.diagnosis)},
        {"prescription",optionalJson(e.prescription)},{"reminder",optionalJson(e.reminder)},
        {"updated_at",optionalJson(e.updatedAt)}};
}
}

WebService::WebService() {
    try {
        open(hospital, hospitalPath);
        Transaction tx(hospital);
        execute(hospital, R"(CREATE TABLE IF NOT EXISTS patients (
            id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, birth_date TEXT NOT NULL,
            age INTEGER NOT NULL, gender TEXT, hometown TEXT, address TEXT, phone TEXT,
            height REAL, weight REAL, bmi REAL);
            CREATE TABLE IF NOT EXISTS checkins (
            checkin_id INTEGER PRIMARY KEY AUTOINCREMENT, patient_id INTEGER NOT NULL,
            department TEXT NOT NULL, checkin_time TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            priority INTEGER NOT NULL CHECK(priority BETWEEN 1 AND 5),
            FOREIGN KEY(patient_id) REFERENCES patients(id) ON DELETE RESTRICT);
            CREATE UNIQUE INDEX IF NOT EXISTS ux_checkins_patient_id ON checkins(patient_id);)");
        for (const auto* column : {"height", "weight", "bmi"}) migrate(hospital, column);
        tx.commit();
        open(priority, priorityPath);
        execute(priority, R"(CREATE TABLE IF NOT EXISTS priority_checkins (
            checkin_id INTEGER PRIMARY KEY, patient_id INTEGER NOT NULL, department TEXT NOT NULL,
            checkin_time TEXT NOT NULL, base_priority INTEGER NOT NULL CHECK(base_priority BETWEEN 1 AND 5),
            current_priority INTEGER NOT NULL CHECK(current_priority BETWEEN 1 AND 5), last_update TEXT);)");
        std::filesystem::create_directories("TRUY_XUAT_BENH_NHAN/db");
        require(DBTaoBang::taoBangTruyXuat(), 500, "Khong tao duoc bang hang doi");
        open(retrieval, retrievalPath);
        execute(retrieval, R"(CREATE TABLE IF NOT EXISTS doctor_state (
            doctor_id TEXT PRIMARY KEY, duty_mode TEXT NOT NULL DEFAULT 'auto',
            busy_until TEXT, busy_reason TEXT NOT NULL DEFAULT '');)");
        std::filesystem::create_directories("DANG_KHAM/db");
        require(exams.mo(retrievalPath, "DANG_KHAM/db/dangKham.db") && exams.taoCauTruc(),
                500, "Khong khoi tao duoc database kham");
    } catch (...) {
        exams.dong();
        sqlite3_close(retrieval); sqlite3_close(priority); sqlite3_close(hospital);
        throw;
    }
}
WebService::~WebService() {
    exams.dong(); sqlite3_close(retrieval); sqlite3_close(priority); sqlite3_close(hospital);
}
std::vector<Patient> WebService::patients() {
    std::vector<Patient> records;
    require(HospitalPersistence::loadPatients(hospital, records), 500, "Khong doc duoc benh nhan");
    return records;
}
Patient WebService::findPatient(int id) {
    auto records = patients();
    auto index = PatientCore::indexPatients(records);
    std::size_t position;
    require(index.find(std::to_string(id), position), 404, "Khong tim thay benh nhan");
    return records[position];
}
Json WebService::listPatients(const std::string& search) {
    Json result = Json::array();
    for (const auto& p : patients())
        if (search.empty() || p.name.find(search) != std::string::npos || p.phone.find(search) != std::string::npos
            || std::to_string(p.id) == search) result.push_back(patientJson(p));
    return result;
}
Json WebService::getPatient(int id) { return patientJson(findPatient(id)); }
Json WebService::savePatient(const Json& data, int id) {
    Transaction tx(hospital);
    Patient p = id ? findPatient(id) : Patient{};
    auto updateText = [&](const char* key, std::string& value, bool required = false) {
        if (data.contains(key) || !id) value = stringField(data, key, required);
    };
    updateText("name", p.name, true); updateText("birth_date", p.birthDate, true);
    updateText("phone", p.phone); updateText("gender", p.gender);
    updateText("hometown", p.hometown); updateText("address", p.address);
    const char* nullableFields[] = {"gender", "hometown", "address", "phone"};
    for (int i = 0; i < 4; ++i) if (data.contains(nullableFields[i])) p.nullFields &= ~(1u << i);
    try { p.age = ageFromBirthDate(p.birthDate); }
    catch (const std::invalid_argument& e) { throw ApiError(400, e.what()); }
    auto measure = [&](const char* key, double& value, double maximum) {
        if (!data.contains(key)) return;
        require(data[key].is_number(), 400, std::string(key) + " phai la so");
        value = data[key].get<double>();
        require(std::isfinite(value) && value >= 0 && value <= maximum, 400, std::string(key) + " khong hop le");
    };
    measure("height", p.height, 300); measure("weight", p.weight, 1000);
    p.bmi = p.height > 0 && p.weight > 0 ? p.weight / std::pow(p.height / 100, 2) : 0;
    require(std::isfinite(p.bmi), 400, "Chieu cao qua nho de tinh BMI");
    auto stmt = prepare(hospital, id ?
        "UPDATE patients SET name=?,birth_date=?,age=?,phone=?,gender=?,hometown=?,address=?,height=?,weight=?,bmi=? WHERE id=?;" :
        "INSERT INTO patients (name,birth_date,age,phone,gender,hometown,address,height,weight,bmi) VALUES (?,?,?,?,?,?,?,?,?,?);");
    bindText(stmt.get(), 1, p.name); bindText(stmt.get(), 2, p.birthDate); sqlite3_bind_int(stmt.get(), 3, p.age);
    const std::string* nullableValues[] = {&p.phone, &p.gender, &p.hometown, &p.address};
    const unsigned nullableBits[] = {8, 1, 2, 4};
    for (int i = 0; i < 4; ++i) {
        if (p.nullFields & nullableBits[i]) sqlite3_bind_null(stmt.get(), i + 4);
        else bindText(stmt.get(), i + 4, *nullableValues[i]);
    }
    sqlite3_bind_double(stmt.get(), 8, p.height); sqlite3_bind_double(stmt.get(), 9, p.weight);
    sqlite3_bind_double(stmt.get(), 10, p.bmi); if (id) sqlite3_bind_int(stmt.get(), 11, id);
    require(sqlite3_step(stmt.get()) == SQLITE_DONE, 500, "Khong luu duoc benh nhan");
    if (!id) p.id = static_cast<int>(sqlite3_last_insert_rowid(hospital));
    tx.commit();
    return patientJson(p);
}
Json WebService::deletePatient(int id) {
    Transaction tx(hospital);
    findPatient(id);
    std::vector<CheckInRecord> records;
    require(HospitalPersistence::loadCheckIns(hospital, records), 500, "Khong doc duoc check-in");
    require(!PatientCore::latestCheckIn(records, id), 409, "Benh nhan da check-in, khong the xoa");
    auto stmt = prepare(hospital, "DELETE FROM patients WHERE id=?;");
    sqlite3_bind_int(stmt.get(), 1, id);
    require(sqlite3_step(stmt.get()) == SQLITE_DONE, 409, "Khong the xoa benh nhan");
    tx.commit(); return {{"deleted",id}};
}
Json WebService::departments() {
    Json result = Json::array();
    for (const auto& d : CauHinhTruyXuat::danhSachKhoa) {
        std::string reason;
        const bool accepting = khoaDangHoatDong(std::string(d.ten), 4, reason);
        result.push_back({{"name",d.ten},{"accepting_checkins",accepting},{"message",reason}});
    }
    return result;
}
Json WebService::checkIn(const Json& data) {
    const int id = intField(data, "patient_id", 1, 2147483647);
    const int level = intField(data, "priority", 1, 5);
    auto department = stringField(data, "department", true);
    require(PatientCore::departmentOrder(department) != 99, 400, "Khoa khong hop le");
    std::string reason;
    if (!khoaDangHoatDong(department, level, reason)) {
        if (data.contains("transfer_to_emergency"))
            require(data["transfer_to_emergency"].is_boolean(), 400, "transfer_to_emergency phai la boolean");
        require(level <= 2 && data.value("transfer_to_emergency", false), 409, reason);
        department = "Khoa Cap cuu";
    }
    Transaction tx(hospital);
    findPatient(id);
    std::vector<CheckInRecord> records;
    require(HospitalPersistence::loadCheckIns(hospital, records), 500, "Khong doc duoc check-in");
    require(!PatientCore::latestCheckIn(records, id), 409, "Benh nhan da check-in");
    auto stmt = prepare(hospital, "INSERT INTO checkins(patient_id,department,priority) VALUES(?,?,?);");
    sqlite3_bind_int(stmt.get(), 1, id); bindText(stmt.get(), 2, department); sqlite3_bind_int(stmt.get(), 3, level);
    require(sqlite3_step(stmt.get()) == SQLITE_DONE, 409, "Khong the check-in");
    const int checkinId = static_cast<int>(sqlite3_last_insert_rowid(hospital));
    require(HospitalPersistence::loadCheckIns(hospital, records), 500, "Khong doc duoc phieu check-in");
    auto* saved = PatientCore::findCheckIn(records, checkinId);
    require(saved != nullptr, 500, "Khong tim thay phieu check-in");
    Json result = checkInJson(*saved);
    tx.commit();
    // Phiếu đã lưu là nguồn chính; tác vụ nền sẽ thử lại nếu đồng bộ tạm lỗi.
    try { syncExams(); } catch (const std::exception&) { result["sync_pending"] = true; }
    return result;
}
Json WebService::listCheckIns() {
    std::vector<CheckInRecord> records;
    require(HospitalPersistence::loadCheckIns(hospital, records), 500, "Khong doc duoc check-in");
    PatientCore::sortCheckIns(records);
    auto people = patients(); const auto index = PatientCore::indexPatients(people);
    Json result = Json::array();
    for (const auto& r : records) {
        auto item = checkInJson(r); std::size_t position;
        if (index.find(std::to_string(r.patientId), position)) item["patient_name"] = people[position].name;
        result.push_back(item);
    }
    return result;
}
void WebService::syncPriority() {
    PrioritySync sync(hospital, priority);
    require(sync.syncAll(), 500, "Dong bo uu tien that bai");
    AutoPriorityHeap heap;
    loadPatients(priority, heap);
    processAuto(priority, heap);
}
Json WebService::queue() {
    // GET chỉ đọc. Đồng bộ/ghi queue được thực hiện bằng POST /api/queue/sync.
    std::vector<HoSoTruyXuat> records;
    require(DBTruyXuat::docDanhSachBenhNhan(records), 500, "Khong doc duoc hang doi");
    if (!records.empty()) {
        std::vector<HoSoTruyXuat> buffer(records.size());
        ThuatToanSapXep::sapXepTron(records, buffer, 0, static_cast<int>(records.size()) - 1);
    }
    auto assigned = assignments(); ExamCore::SessionIndex ids;
    for (std::size_t i = 0; i < assigned.size(); ++i) ids.put(assigned[i].CheckinId, i);
    auto people = patients(); auto index = PatientCore::indexPatients(people);
    Json result = Json::array();
    for (const auto& r : records) {
        std::size_t position;
        if (ids.find(r.checkinId, position)) continue;
        Json item = {{"checkin_id",r.checkinId},{"patient_id",r.patientId},{"department",r.department},
            {"base_priority",r.basePriority},{"current_priority",r.currentPriority},
            {"checkin_time",r.checkinTime},{"last_update",r.lastUpdate}};
        if (index.find(std::to_string(r.patientId), position)) item["patient_name"] = people[position].name;
        result.push_back(item);
    }
    return result;
}
Json WebService::changePriority(int id, const Json& data) {
    const int level = intField(data, "priority", 1, 5);
    syncPriority(); PriorityManager manager(priority);
    int base, current;
    require(manager.getPriority(id, base, current), 404, "Khong tim thay check-in");
    for (const auto& r : assignments()) require(r.CheckinId != id, 409, "Benh nhan da duoc phan bac si");
    if (current != level) require(manager.updatePriority(id, level), 409, "Khong cap nhat duoc uu tien");
    return {{"checkin_id",id},{"base_priority",base},{"current_priority",level}};
}
Json WebService::listDoctors() {
    QuanLyBacSi manager;
    require(manager.DocCSV(doctorsPath), 500, "Khong doc duoc CSV bac si");
    Json saved = Json::object();
    auto stmt = prepare(retrieval, "SELECT doctor_id,duty_mode,busy_until,busy_reason FROM doctor_state;");
    int rc;
    while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW)
        saved[HospitalPersistence::text(stmt.get(), 0)] = {
            {"duty_mode",HospitalPersistence::text(stmt.get(), 1)},
            {"busy_until",HospitalPersistence::text(stmt.get(), 2)},
            {"busy_reason",HospitalPersistence::text(stmt.get(), 3)}};
    require(rc == SQLITE_DONE, 500, "Khong doc duoc trang thai bac si");
    const auto now = time(nullptr);
    auto active = listExams(true);
    Json result = Json::array();
    int capCuuIndex = 0;
    for (const auto& d : manager.LayDanhSach()) {
        const int phase = d.khoaChuyenMon == "Khoa Cap cuu" ? capCuuIndex++ % 6 : 0;
        const auto settings = saved.value(d.id, Json::object());
        const auto mode = settings.value("duty_mode", "auto");
        const auto until = settings.value("busy_until", "");
        time_t busyEnd = 0;
        const bool busy = ExamCore::parseTime(until, busyEnd) && busyEnd > now;
        const bool duty = mode == "on_duty" || (mode == "auto" &&
            (d.khoaChuyenMon == "Khoa Cap cuu" ? ThoiGian::DangTrucCapCuu(now, phase) : ThoiGian::DangTrongCaThuong(now)));
        bool examining = false;
        for (const auto& e : active) if (e["doctor_id"] == d.id) examining = true;
        Json shifts = Json::array();
        for (const auto& shift : ThoiGian::LichTruc(now, d.khoaChuyenMon == "Khoa Cap cuu", phase))
            shifts.push_back({{"date",shift.ngay},{"start_time",ThoiGian::DinhDang(shift.batDau)},
                {"end_time",ThoiGian::DinhDang(shift.ketThuc)},
                {"is_current",now >= shift.batDau && now < shift.ketThuc}});
        result.push_back({{"id",d.id},{"name",d.name},{"department",d.khoaChuyenMon},
            {"experience_years",d.ExpYears},{"duty_mode",mode},{"on_duty",duty},{"shifts",shifts},
            {"shift_period_start",ThoiGian::DinhDangNgay(now)},
            {"shift_rule",d.khoaChuyenMon == "Khoa Cap cuu" ? "three_8h_rotating_days_off" : "weekday_split"},
            {"busy",busy},{"busy_until",busy ? Json(until) : Json(nullptr)},
            {"busy_reason",busy ? settings.value("busy_reason", "") : ""},
            {"status",examining ? "examining" : busy ? "busy" : duty ? "on_duty" : "off_duty"}});
    }
    return result;
}
Json WebService::updateDoctor(const std::string& id, const Json& data) {
    auto doctors = listDoctors();
    auto found = std::find_if(doctors.begin(), doctors.end(), [&](const Json& d) { return d["id"] == id; });
    require(found != doctors.end(), 404, "Khong tim thay bac si");
    require(data.contains("duty_mode") || data.contains("busy_minutes"), 400, "Thieu trang thai can cap nhat");
    const auto mode = data.contains("duty_mode") ? stringField(data, "duty_mode", true) : (*found)["duty_mode"].get<std::string>();
    require(mode == "auto" || mode == "on_duty" || mode == "off_duty", 400, "Trang thai ca truc khong hop le");
    auto until = (*found)["busy_until"].is_string() ? (*found)["busy_until"].get<std::string>() : "";
    auto reason = (*found)["busy_reason"].get<std::string>();
    if (data.contains("busy_minutes")) {
        const auto minutes = intField(data, "busy_minutes", 0, 1440);
        until = minutes ? ThoiGian::DinhDang(time(nullptr) + minutes * 60) : "";
        reason = minutes ? stringField(data, "busy_reason", true) : "";
    }
    Transaction tx(retrieval);
    auto stmt = prepare(retrieval, "INSERT INTO doctor_state(doctor_id,duty_mode,busy_until,busy_reason) VALUES(?,?,?,?) "
        "ON CONFLICT(doctor_id) DO UPDATE SET duty_mode=excluded.duty_mode,busy_until=excluded.busy_until,busy_reason=excluded.busy_reason;");
    bindText(stmt.get(), 1, id); bindText(stmt.get(), 2, mode); bindText(stmt.get(), 3, until); bindText(stmt.get(), 4, reason);
    require(sqlite3_step(stmt.get()) == SQLITE_DONE, 500, "Khong luu duoc trang thai bac si");

    int returned = 0;
    if (!until.empty() || mode == "off_duty") {
        const auto now = time(nullptr);
        for (const auto& appointment : assignments()) {
            if (appointment.DoctorId != id) continue;
            time_t start = 0;
            if (appointment.StartTime.empty() ||
                (ExamCore::parseTime(appointment.StartTime, start) && start > now))
                ++returned;
        }
        require(exams.xoaCaChuaBatDauCuaBacSi(id), 500, "Khong tra duoc ca ve hang doi");
    }

    tx.commit();
    return {{"doctor_id",id},{"returned_to_queue",returned}};
}
std::vector<BenhNhanKham> WebService::assignments() {
    std::vector<BenhNhanKham> result;
    require(exams.docPhanBacSi(result), 500, "Khong doc duoc lich bac si");
    return result;
}
Json WebService::listAssignments() {
    Json result = Json::array();
    for (const auto& r : assignments()) result.push_back({{"checkin_id",r.CheckinId},{"patient_id",r.PatientId},
        {"department",r.khoa},{"doctor_department",r.KhoaBacSi},{"doctor_id",r.DoctorId},{"doctor_name",r.DoctorName},
        {"start_time",r.StartTime},{"planned_end_time",r.EndTime},{"duration_minutes",r.ExamDuration},
        {"status",r.Status},{"note",r.Note}});
    return result;
}
Json WebService::schedule(const Json& data) {
    auto department = stringField(data, "department");
    const auto doctorId = stringField(data, "doctor_id", data.contains("doctor_id"));
    require(department.empty() || PatientCore::departmentOrder(department) != 99, 400, "Khoa khong hop le");
    std::vector<std::string> allowed, duty;
    bool doctorFound = doctorId.empty();
    for (const auto& doctor : listDoctors()) {
        const auto id = doctor["id"].get<std::string>();
        if (!doctorId.empty() && doctorId != id) continue;
        if (!doctorId.empty()) {
            doctorFound = true;
            require(department.empty() || department == doctor["department"], 400, "Bac si khong thuoc khoa da chon");
            department = doctor["department"].get<std::string>();
            require(!doctor["busy"].get<bool>() && doctor["status"] != "examining" && doctor["duty_mode"] != "off_duty",
                409, "Bac si dang ban, dang kham hoac da nghi ca");
        }
        if (doctor["busy"].get<bool>() || doctor["status"] == "examining" || doctor["duty_mode"] == "off_duty") continue;
        allowed.push_back(id);
        if (doctor["duty_mode"] == "on_duty") duty.push_back(id);
    }
    require(doctorFound, 404, "Khong tim thay bac si");
    syncPriority();
    QuanLyHangDoi queueManager;
    require(queueManager.taiVaXuLyBenhNhan(), 500, "Khong dong bo duoc hang doi");
    QuanLyKhamBenh manager;
    require(manager.KhoiDongWeb(retrievalPath, doctorsPath, assignments(), allowed, duty), 500, "Khong khoi dong duoc phan bac si");
    require(department.empty() ? manager.XuLyTatCaKhoa() : manager.XuLyKhoa(department),
        500, "Phan bac si that bai; kiem tra lich da luu truoc khi thu lai");
    require(exams.ghiPhanBacSi(manager.LayKetQua()), 500, "Khong luu duoc ket qua phan bac si");
    syncExams();
    return {{"assigned_count",manager.LayKetQua().size()},{"assignments",listAssignments()}};
}
Json WebService::syncExams() {
    syncPriority();
    QuanLyHangDoi manager;
    require(manager.taiVaXuLyBenhNhan(), 500, "Khong dong bo duoc hang doi");
    std::vector<std::string> blocked;
    for (const auto& doctor : listDoctors())
        if (doctor["busy"].get<bool>() || doctor["duty_mode"] == "off_duty") blocked.push_back(doctor["id"]);
    require(exams.dongBoTuXepBacSi(blocked), 500, "Khong dong bo duoc ca kham");
    return {{"queue",queue()},{"active_exams",listExams(true)}};
}
Json WebService::listExams(bool activeOnly) {
    std::vector<ExamSession> records;
    require(exams.docDanhSach(records), 500, "Khong doc duoc ca kham");
    if (activeOnly) records = ExamCore::activeSessions(records);
    Json result = Json::array();
    for (const auto& r : records) result.push_back(examJson(r));
    return result;
}
Json WebService::diagnosis(int id, const Json& data) {
    const auto diagnosis = stringField(data, "diagnosis", true);
    const auto prescription = stringField(data, "prescription");
    const auto reminder = stringField(data, "reminder");
    std::vector<ExamSession> records;
    require(exams.docDanhSach(records), 500, "Khong doc duoc ca kham");
    require(ExamCore::findActive(records, id) != nullptr, 409, "Ca kham khong dang hoat dong");
    require(exams.luuChanDoan(id, diagnosis, prescription, reminder), 409, "Khong luu duoc chan doan");
    return {{"checkin_id",id},{"saved",true}};
}
Json WebService::finishExam(int id) {
    std::vector<ExamSession> records;
    require(exams.docDanhSach(records), 500, "Khong doc duoc ca kham");
    require(ExamCore::findActive(records, id) != nullptr, 409, "Ca kham khong dang hoat dong");
    require(exams.ketThucPhien(id), 409, "Khong ket thuc duoc ca kham");
    return {{"checkin_id",id},{"finished",true}};
}
}
