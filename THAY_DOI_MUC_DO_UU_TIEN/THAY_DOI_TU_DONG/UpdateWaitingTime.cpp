#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <sqlite3.h>

#include "WorkingTime.h"

using namespace std;


// Chuyen chuoi thoi gian SQLite thanh time_t
bool parseTime(const string& text, time_t& result)
{
    tm timeInfo = {};

    stringstream ss(text);

    ss >> get_time(
        &timeInfo,
        "%Y-%m-%d %H:%M:%S"
    );

    if (ss.fail())
    {
        return false;
    }

    timeInfo.tm_isdst = -1;

    result = mktime(&timeInfo);

    return result != static_cast<time_t>(-1);
}


// Cap nhat tong thoi gian cho
bool updateWaitingTime(sqlite3* db)
{
    // Doc ma check-in va thoi gian check-in
    const char* selectSql = R"(

        SELECT
            checkin_id,
            checkin_time

        FROM priority_checkins;

    )";

    sqlite3_stmt* selectStmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            selectSql,
            -1,
            &selectStmt,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Loi doc priority_checkins: "
             << sqlite3_errmsg(db) << '\n';

        return false;
    }


    // Them moi hoac cap nhat thoi gian cho
    const char* updateSql = R"(

        INSERT INTO waiting_time_progress
        (
            checkin_id,
            waiting_seconds
        )

        VALUES (?, ?)

        ON CONFLICT(checkin_id)

        DO UPDATE SET
            waiting_seconds =
                excluded.waiting_seconds;

    )";

    sqlite3_stmt* updateStmt = nullptr;

    if (sqlite3_prepare_v2(
            db,
            updateSql,
            -1,
            &updateStmt,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Loi truy van waiting_time_progress: "
             << sqlite3_errmsg(db) << '\n';

        sqlite3_finalize(selectStmt);

        return false;
    }


    // Dung cung mot thoi diem cho tat ca benh nhan
    time_t now = time(nullptr);

    int count = 0;

    // Bat dau transaction
    if (sqlite3_exec(
            db,
            "BEGIN IMMEDIATE;",
            nullptr,
            nullptr,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Loi bat dau transaction.\n";

        sqlite3_finalize(selectStmt);
        sqlite3_finalize(updateStmt);

        return false;
    }

    bool success = true;

    int result;

    // Duyet tung luot check-in
    while ((result = sqlite3_step(selectStmt)) == SQLITE_ROW)
    {
        int checkinId =
            sqlite3_column_int(selectStmt, 0);

        const unsigned char* text =
            sqlite3_column_text(selectStmt, 1);

        if (text == nullptr)
        {
            cerr << "Thieu thoi gian check-in: "
                 << checkinId << '\n';

            success = false;
            break;
        }

        string checkinTime =
            reinterpret_cast<const char*>(text);

        time_t checkinTimestamp;

        if (!parseTime(checkinTime, checkinTimestamp))
        {
            cerr << "Thoi gian khong hop le: "
                 << checkinId << '\n';

            success = false;
            break;
        }

        // Chi tinh thoi gian trong gio lam viec
        long long waitingSeconds =
            calculateWorkingSeconds(
                checkinTimestamp,
                now
            );

        sqlite3_reset(updateStmt);
        sqlite3_clear_bindings(updateStmt);

        sqlite3_bind_int(
            updateStmt,
            1,
            checkinId
        );

        sqlite3_bind_int64(
            updateStmt,
            2,
            waitingSeconds
        );

        if (sqlite3_step(updateStmt) != SQLITE_DONE)
        {
            cerr << "Loi cap nhat checkin_id = "
                 << checkinId << '\n';

            success = false;
            break;
        }

        count++;
    }

    if (result != SQLITE_DONE)
    {
        success = false;
    }

    sqlite3_finalize(selectStmt);
    sqlite3_finalize(updateStmt);


    // Neu co loi, huy toan bo lan cap nhat
    if (!success)
    {
        sqlite3_exec(
            db,
            "ROLLBACK;",
            nullptr,
            nullptr,
            nullptr
        );

        return false;
    }

    // Luu thay doi
    if (sqlite3_exec(
            db,
            "COMMIT;",
            nullptr,
            nullptr,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Loi luu du lieu: "
             << sqlite3_errmsg(db) << '\n';

        sqlite3_exec(
            db,
            "ROLLBACK;",
            nullptr,
            nullptr,
            nullptr
        );

        return false;
    }

    cout << "Cap nhat thoi gian cho thanh cong.\n";

    cout << "So luot check-in: "
         << count << '\n';

    return true;
}


int main()
{
    sqlite3* db = nullptr;

    const char* dbPath =
        "THAY_DOI_MUC_DO_UU_TIEN/priority.db";

    // Chi mo database da ton tai
    if (sqlite3_open_v2(
            dbPath,
            &db,
            SQLITE_OPEN_READWRITE,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Khong mo duoc priority.db.\n";

        if (db != nullptr)
        {
            sqlite3_close(db);
        }

        return 1;
    }

    // Bat kiem tra khoa ngoai
    if (sqlite3_exec(
            db,
            "PRAGMA foreign_keys = ON;",
            nullptr,
            nullptr,
            nullptr
        ) != SQLITE_OK)
    {
        cerr << "Loi bat khoa ngoai.\n";

        sqlite3_close(db);

        return 1;
    }

    bool success = updateWaitingTime(db);

    sqlite3_close(db);

    return success ? 0 : 1;
}