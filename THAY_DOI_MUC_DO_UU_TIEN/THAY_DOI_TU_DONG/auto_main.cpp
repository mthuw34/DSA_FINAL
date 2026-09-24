#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <chrono>
#include <sqlite3.h>

#include "WorkingTime.h"
#include "HANG_DOI_UU_TIEN/AutoPriorityHeap.h"

using namespace std;


// =====================================================
// CHUYEN CHUOI THOI GIAN SQLite SANG time_t
// =====================================================

bool parseTime(
    const string& text,
    time_t& result
)
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


// =====================================================
// CAP NHAT TONG THOI GIAN CHO
// =====================================================

bool updateWaitingTime(sqlite3* db)
{
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
        cerr
            << "Loi doc priority_checkins: "
            << sqlite3_errmsg(db)
            << '\n';

        return false;
    }


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
        cerr
            << "Loi waiting_time_progress: "
            << sqlite3_errmsg(db)
            << '\n';

        sqlite3_finalize(selectStmt);

        return false;
    }


    // Dung cung mot moc thoi gian
    // cho tat ca benh nhan trong lan cap nhat nay
    time_t now = time(nullptr);


    if (sqlite3_exec(
            db,
            "BEGIN IMMEDIATE;",
            nullptr,
            nullptr,
            nullptr
        ) != SQLITE_OK)
    {
        sqlite3_finalize(selectStmt);
        sqlite3_finalize(updateStmt);

        return false;
    }


    bool success = true;

    int result;


    while (
        (result = sqlite3_step(selectStmt))
        == SQLITE_ROW
    )
    {
        int checkinId =
            sqlite3_column_int(
                selectStmt,
                0
            );


        const unsigned char* text =
            sqlite3_column_text(
                selectStmt,
                1
            );


        if (text == nullptr)
        {
            success = false;

            break;
        }


        string checkinTime =
            reinterpret_cast<const char*>(
                text
            );


        time_t checkinTimestamp;


        if (!parseTime(
                checkinTime,
                checkinTimestamp
            ))
        {
            cerr
                << "Sai thoi gian cua checkin ID "
                << checkinId
                << '\n';

            success = false;

            break;
        }


        // Chi tinh thoi gian cho hop le
        // trong gio lam viec

        long long waitingSeconds =
            calculateWorkingSeconds(
                checkinTimestamp,
                now
            );


        sqlite3_reset(updateStmt);

        sqlite3_clear_bindings(
            updateStmt
        );


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


        if (
            sqlite3_step(updateStmt)
            != SQLITE_DONE
        )
        {
            success = false;

            break;
        }
    }


    if (result != SQLITE_DONE)
    {
        success = false;
    }


    sqlite3_finalize(selectStmt);

    sqlite3_finalize(updateStmt);


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


    if (sqlite3_exec(
            db,
            "COMMIT;",
            nullptr,
            nullptr,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }


    return true;
}


// =====================================================
// LAY MOC 60 PHUT CUOI CUNG DA XU LY
// =====================================================

int getLastProcessedHour(
    sqlite3* db,
    int checkinId
)
{
    const char* sql = R"(

        SELECT MAX(waiting_hour)

        FROM auto_priority_history

        WHERE checkin_id = ?;

    )";


    sqlite3_stmt* stmt = nullptr;


    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        return 0;
    }


    sqlite3_bind_int(
        stmt,
        1,
        checkinId
    );


    int lastHour = 0;


    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (
            sqlite3_column_type(
                stmt,
                0
            )
            != SQLITE_NULL
        )
        {
            lastHour =
                sqlite3_column_int(
                    stmt,
                    0
                );
        }
    }


    sqlite3_finalize(stmt);


    return lastHour;
}


// =====================================================
// LAY PRIORITY HIEN TAI
// =====================================================

bool getCurrentPriority(
    sqlite3* db,
    int checkinId,
    int& currentPriority
)
{
    const char* sql = R"(

        SELECT current_priority

        FROM priority_checkins

        WHERE checkin_id = ?;

    )";


    sqlite3_stmt* stmt = nullptr;


    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }


    sqlite3_bind_int(
        stmt,
        1,
        checkinId
    );


    if (
        sqlite3_step(stmt)
        != SQLITE_ROW
    )
    {
        sqlite3_finalize(stmt);

        return false;
    }


    currentPriority =
        sqlite3_column_int(
            stmt,
            0
        );


    sqlite3_finalize(stmt);


    return true;
}


// =====================================================
// NAP CAC BENH NHAN CAN THEO DOI VAO MIN-HEAP
// =====================================================

bool loadHeap(
    sqlite3* db,
    AutoPriorityHeap& heap
)
{
    const char* sql = R"(

        SELECT
            p.checkin_id,
            w.waiting_seconds

        FROM priority_checkins p

        JOIN waiting_time_progress w
            ON p.checkin_id = w.checkin_id

        WHERE p.current_priority > 1;

    )";


    sqlite3_stmt* stmt = nullptr;


    if (sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK)
    {
        cerr
            << "Loi doc du lieu Heap: "
            << sqlite3_errmsg(db)
            << '\n';

        return false;
    }


    while (
        sqlite3_step(stmt)
        == SQLITE_ROW
    )
    {
        int checkinId =
            sqlite3_column_int(
                stmt,
                0
            );


        long long waitingSeconds =
            sqlite3_column_int64(
                stmt,
                1
            );


        // Moc cuoi cung da duoc xu ly

        int lastHour =
            getLastProcessedHour(
                db,
                checkinId
            );


        // Moc can xu ly tiep theo

        int nextHour =
            lastHour + 1;


        long long nextThreshold =
            static_cast<long long>(
                nextHour
            )
            * 3600;


        // <= 0 nghia la da du moc

        long long remainingSeconds =
            nextThreshold
            - waitingSeconds;


        AutoPriorityItem item;


        item.checkinId =
            checkinId;

        item.waitingSeconds =
            waitingSeconds;

        item.nextHour =
            nextHour;

        item.remainingSeconds =
            remainingSeconds;


        heap.insert(item);
    }


    sqlite3_finalize(stmt);


    return true;
}


// =====================================================
// XU LY MOT MOC 60 PHUT
// =====================================================

bool processOneHour(
    sqlite3* db,
    int checkinId,
    int waitingHour
)
{
    int currentPriority;


    if (!getCurrentPriority(
            db,
            checkinId,
            currentPriority
        ))
    {
        return false;
    }


    // Muc 1 la cao nhat

    if (currentPriority <= 1)
    {
        return true;
    }


    int newPriority =
        currentPriority - 1;


    // =================================================
    // BAT DAU TRANSACTION
    // =================================================

    if (sqlite3_exec(
            db,
            "BEGIN IMMEDIATE;",
            nullptr,
            nullptr,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }


    // =================================================
    // GHI MOC VAO LICH SU TRUOC
    // =================================================

    const char* historySql = R"(

        INSERT INTO auto_priority_history
        (
            checkin_id,
            waiting_hour,
            updated_at
        )

        VALUES
        (
            ?,
            ?,
            datetime('now', 'localtime')
        );

    )";


    sqlite3_stmt* historyStmt =
        nullptr;


    if (sqlite3_prepare_v2(
            db,
            historySql,
            -1,
            &historyStmt,
            nullptr
        ) != SQLITE_OK)
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


    sqlite3_bind_int(
        historyStmt,
        1,
        checkinId
    );


    sqlite3_bind_int(
        historyStmt,
        2,
        waitingHour
    );


    if (
        sqlite3_step(historyStmt)
        != SQLITE_DONE
    )
    {
        sqlite3_finalize(
            historyStmt
        );

        sqlite3_exec(
            db,
            "ROLLBACK;",
            nullptr,
            nullptr,
            nullptr
        );

        return false;
    }


    sqlite3_finalize(
        historyStmt
    );


    // =================================================
    // CAP NHAT PRIORITY
    // =================================================

    const char* updateSql = R"(

        UPDATE priority_checkins

        SET
            current_priority = ?,

            last_update =
                datetime(
                    'now',
                    'localtime'
                )

        WHERE checkin_id = ?;

    )";


    sqlite3_stmt* updateStmt =
        nullptr;


    if (sqlite3_prepare_v2(
            db,
            updateSql,
            -1,
            &updateStmt,
            nullptr
        ) != SQLITE_OK)
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


    sqlite3_bind_int(
        updateStmt,
        1,
        newPriority
    );


    sqlite3_bind_int(
        updateStmt,
        2,
        checkinId
    );


    if (
        sqlite3_step(updateStmt)
        != SQLITE_DONE
    )
    {
        sqlite3_finalize(
            updateStmt
        );

        sqlite3_exec(
            db,
            "ROLLBACK;",
            nullptr,
            nullptr,
            nullptr
        );

        return false;
    }


    sqlite3_finalize(
        updateStmt
    );


    // =================================================
    // COMMIT
    // =================================================

    if (sqlite3_exec(
            db,
            "COMMIT;",
            nullptr,
            nullptr,
            nullptr
        ) != SQLITE_OK)
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


    // Chi hien khi co thay doi that su

    cout
        << "Checkin ID "
        << checkinId
        << ": "
        << currentPriority
        << " -> "
        << newPriority
        << '\n';


    return true;
}


// =====================================================
// KIEM TRA VA CAP NHAT PRIORITY
// =====================================================

bool updateAutoPriority(
    sqlite3* db
)
{
    // Ngoai gio lam viec:
    // khong duoc thay doi muc uu tien

    time_t now =
        time(nullptr);


    if (!isWorkingTime(now))
    {
        return true;
    }


    AutoPriorityHeap heap;


    if (!loadHeap(
            db,
            heap
        ))
    {
        return false;
    }


    AutoPriorityItem item;


    // =================================================
    // MIN-HEAP
    // =================================================

    while (heap.peek(item))
    {
        // Root chua den han
        // => tat ca phan tu con lai cung chua den han

        if (item.remainingSeconds > 0)
        {
            break;
        }


        heap.extractMin(item);


        // Tong so moc 60 phut
        // da hoan thanh

        long long completedHours =
            item.waitingSeconds
            / 3600;


        // Moc da xu ly gan nhat

        int lastHour =
            getLastProcessedHour(
                db,
                item.checkinId
            );


        // =================================================
        // CO THE CO NHIEU MOC CHUA DUOC XU LY
        // =================================================

        for (
            int hour = lastHour + 1;
            hour <= completedHours;
            hour++
        )
        {
            int currentPriority;


            if (!getCurrentPriority(
                    db,
                    item.checkinId,
                    currentPriority
                ))
            {
                return false;
            }


            // Dat muc cao nhat thi dung

            if (currentPriority <= 1)
            {
                break;
            }


            if (!processOneHour(
                    db,
                    item.checkinId,
                    hour
                ))
            {
                return false;
            }
        }
    }


    return true;
}


// =====================================================
// MAIN
// =====================================================

int main()
{
    sqlite3* db = nullptr;


    const char* dbPath =
        "THAY_DOI_MUC_DO_UU_TIEN/priority.db";


    if (sqlite3_open_v2(
            dbPath,
            &db,
            SQLITE_OPEN_READWRITE,
            nullptr
        ) != SQLITE_OK)
    {
        cerr
            << "Khong mo duoc priority.db.\n";


        if (db != nullptr)
        {
            sqlite3_close(db);
        }


        return 1;
    }


    // Bat khoa ngoai

    sqlite3_exec(
        db,
        "PRAGMA foreign_keys = ON;",
        nullptr,
        nullptr,
        nullptr
    );


    cout
        << "HE THONG CAP NHAT UU TIEN TU DONG DANG CHAY...\n";

    cout
        << "Nhan Ctrl + C de dung chuong trinh.\n\n";


    // =================================================
    // CHAY LIEN TUC
    // =================================================

    while (true)
    {
        // ---------------------------------------------
        // 1. CAP NHAT TONG THOI GIAN CHO
        // ---------------------------------------------

        if (!updateWaitingTime(db))
        {
            cerr
                << "Loi cap nhat thoi gian cho.\n";

            break;
        }


        // ---------------------------------------------
        // 2. KIEM TRA VA TANG PRIORITY
        // ---------------------------------------------

        if (!updateAutoPriority(db))
        {
            cerr
                << "Loi cap nhat muc do uu tien.\n";

            break;
        }


        // ---------------------------------------------
        // 3. CHO 60 GIAY
        // ---------------------------------------------

        this_thread::sleep_for(
            chrono::seconds(60)
        );
    }


    sqlite3_close(db);


    return 0;
}