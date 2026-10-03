#include "TruyXuat.h"

using namespace std;

bool DocDuLieu::layDanhSachBenhNhan(
    sqlite3* priorityDatabase,
    vector<HoSoTruyXuat>& outRecords
) {
    const char* query = R"(
        SELECT checkin_id, patient_id, department, checkin_time,
               base_priority, current_priority, last_update,
               CASE department
                   WHEN 'Khoa Cap cuu' THEN 1 
                   WHEN 'Khoa Noi' THEN 2
                   WHEN 'Khoa Ngoai' THEN 3 
                   WHEN 'Khoa Tim mach' THEN 4
                   WHEN 'Khoa Nhi' THEN 5 
                   WHEN 'Khoa San' THEN 6
                   WHEN 'Khoa Tai Mui Hong' THEN 7 
                   WHEN 'Khoa Mat' THEN 8
                   WHEN 'Khoa Da lieu' THEN 9 
                   WHEN 'Khoa Than kinh' THEN 10
                   ELSE 0
               END
        FROM priority_checkins;
    )";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(priorityDatabase, query, -1, &statement, nullptr) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return false;
    }

    int result = SQLITE_ROW;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        HoSoTruyXuat record;
        record.checkinId = sqlite3_column_int(statement, 0);
        record.patientId = sqlite3_column_int(statement, 1);
        record.department = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
        record.departmentOrder = sqlite3_column_int(statement, 7);

        if (record.departmentOrder == 0) {
            record.departmentOrder = ThuatToanSapXep::layThuTuKhoa(record.department);
        }

        const unsigned char* checkinTimeText = sqlite3_column_text(statement, 3);
        record.checkinTime = checkinTimeText ? reinterpret_cast<const char*>(checkinTimeText) : "";

        record.basePriority = sqlite3_column_int(statement, 4);
        record.currentPriority = sqlite3_column_int(statement, 5);
        const unsigned char* lastUpdateText = sqlite3_column_text(statement, 6);
        record.lastUpdate = lastUpdateText ? reinterpret_cast<const char*>(lastUpdateText) : "";
        outRecords.push_back(record);
    }

    bool success = (result == SQLITE_DONE);
    sqlite3_finalize(statement);
    return success;
}