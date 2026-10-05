#pragma once
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <stdexcept>
#include <mutex>
#include "../DANG_KHAM/src/DatabaseDangKham.h"
#include "../QUAN_LY_BENH_NHAN/src/PatientCore.h"
#include "../SAP_XEP_BAC_SI/src/QUAN_LY_THONG_TIN/BenhNhanKham.h"

namespace hospital_web {
using Json = nlohmann::json;
struct ApiError : std::runtime_error {
    int status;
    ApiError(int code, const std::string& message) : std::runtime_error(message), status(code) {}
};

// Mỗi request được khóa tại tầng API vì các module dùng chung SQLite và lịch bác sĩ.
class WebService {
    sqlite3* hospital = nullptr;
    sqlite3* priority = nullptr;
    DatabaseDangKham exams;
    std::vector<Patient> patients();
    Patient findPatient(int id);
    std::vector<BenhNhanKham> assignments();
    void syncPriority();
public:
    std::mutex mutex;
    WebService();
    ~WebService();
    WebService(const WebService&) = delete;
    WebService& operator=(const WebService&) = delete;
    Json listPatients(const std::string& search = "");
    Json getPatient(int id);
    Json savePatient(const Json& data, int id = 0);
    Json deletePatient(int id);
    Json departments();
    Json checkIn(const Json& data);
    Json listCheckIns();
    Json queue();
    Json changePriority(int id, const Json& data);
    Json listDoctors();
    Json updateDoctor(const std::string& id, const Json& data);
    Json schedule(const Json& data);
    Json listAssignments();
    Json syncExams();
    Json listExams(bool activeOnly);
    Json diagnosis(int id, const Json& data);
    Json finishExam(int id);
};
}
