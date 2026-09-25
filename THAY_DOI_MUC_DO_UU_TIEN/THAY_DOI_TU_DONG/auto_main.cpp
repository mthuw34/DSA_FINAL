#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <chrono>
#include <conio.h>
#include <sqlite3.h>

#include "WorkingTime.h"
#include "HANG_DOI_UU_TIEN/AutoPriorityHeap.h"

using namespace std;


// ========================================
// 1 MOC = 90 PHUT = 5400 GIAY
// ========================================

const long long THOI_GIAN_TANG = 90 * 60;


// ========================================
// DOI CHUOI THOI GIAN SANG time_t
// ========================================

bool parseTime(
    const string& text,
    time_t& result
)
{
    tm t = {};

    stringstream ss(text);

    ss >> get_time(
        &t,
        "%Y-%m-%d %H:%M:%S"
    );

    if (ss.fail())
        return false;

    t.tm_isdst = -1;

    result = mktime(&t);

    return true;
}


// ========================================
// CAP NHAT waiting_seconds
// ========================================

bool updateWaitingTime(sqlite3* db)
{
    const char* selectSql = R"(

        SELECT
            checkin_id,
            checkin_time

        FROM priority_checkins;

    )";


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


    sqlite3_stmt* selectStmt = nullptr;
    sqlite3_stmt* updateStmt = nullptr;


    if (sqlite3_prepare_v2(
            db,
            selectSql,
            -1,
            &selectStmt,
            nullptr
        ) != SQLITE_OK)
    {
        return false;
    }


    if (sqlite3_prepare_v2(
            db,
            updateSql,
            -1,
            &updateStmt,
            nullptr
        ) != SQLITE_OK)
    {
        sqlite3_finalize(selectStmt);
        return false;
    }


    time_t now = time(nullptr);


    while (
        sqlite3_step(selectStmt)
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
            continue;


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
            continue;
        }


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


        sqlite3_step(updateStmt);
    }


    sqlite3_finalize(selectStmt);
    sqlite3_finalize(updateStmt);


    return true;
}


// ========================================
// LAY MOC CUOI CUNG DA XU LY
// ========================================

int getLastPeriod(
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


    int lastPeriod = 0;


    if (
        sqlite3_step(stmt)
        == SQLITE_ROW
    )
    {
        if (
            sqlite3_column_type(
                stmt,
                0
            )
            != SQLITE_NULL
        )
        {
            lastPeriod =
                sqlite3_column_int(
                    stmt,
                    0
                );
        }
    }


    sqlite3_finalize(stmt);


    return lastPeriod;
}


// ========================================
// LAY PRIORITY HIEN TAI
// ========================================

int getCurrentPriority(
    sqlite3* db,
    int checkinId
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
        return -1;
    }


    sqlite3_bind_int(
        stmt,
        1,
        checkinId
    );


    int priority = -1;


    if (
        sqlite3_step(stmt)
        == SQLITE_ROW
    )
    {
        priority =
            sqlite3_column_int(
                stmt,
                0
            );
    }


    sqlite3_finalize(stmt);


    return priority;
}


// ========================================
// NAP DU LIEU VAO MIN-HEAP
// ========================================

void loadHeap(
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
        return;
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


        int lastPeriod =
            getLastPeriod(
                db,
                checkinId
            );


        int nextPeriod =
            lastPeriod + 1;


        long long nextThreshold =
            static_cast<long long>(
                nextPeriod
            )
            * THOI_GIAN_TANG;


        long long remainingSeconds =
            nextThreshold
            - waitingSeconds;


        AutoPriorityItem item;

        item.checkinId =
            checkinId;

        item.waitingSeconds =
            waitingSeconds;

        item.nextHour =
            nextPeriod;

        item.remainingSeconds =
            remainingSeconds;


        heap.insert(item);
    }


    sqlite3_finalize(stmt);
}


// ========================================
// TANG 1 CAP UU TIEN
// ========================================

bool increasePriority(
    sqlite3* db,
    int checkinId,
    int period
)
{
    int currentPriority =
        getCurrentPriority(
            db,
            checkinId
        );


    if (currentPriority <= 1)
        return false;


    int newPriority =
        currentPriority - 1;


    // ====================================
    // CAP NHAT PRIORITY
    // ====================================

    const char* updateSql = R"(

        UPDATE priority_checkins

        SET
            current_priority = ?,

            last_update =
                datetime('now', 'localtime')

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


    sqlite3_step(updateStmt);

    sqlite3_finalize(updateStmt);


    // ====================================
    // GHI LAI MOC DA XU LY
    // ====================================

    const char* historySql = R"(

        INSERT OR IGNORE INTO
        auto_priority_history
        (
            checkin_id,
            waiting_hour
        )

        VALUES (?, ?);

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
        period
    );


    sqlite3_step(historyStmt);

    sqlite3_finalize(historyStmt);


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


// ========================================
// XU LY TU DONG
// ========================================

void processAutoPriority(
    sqlite3* db
)
{
    // Cap nhat tong thoi gian cho

    updateWaitingTime(db);


    // Tao Heap moi

    AutoPriorityHeap heap;


    loadHeap(
        db,
        heap
    );


    AutoPriorityItem item;


    // Lay benh nhan den han som nhat

    while (heap.peek(item))
    {
        // Root chua den han
        // => cac phan tu sau cung chua den han

        if (
            item.remainingSeconds > 0
        )
        {
            break;
        }


        heap.extractMin(item);


        int currentPriority =
            getCurrentPriority(
                db,
                item.checkinId
            );


        if (currentPriority <= 1)
        {
            continue;
        }


        // Tong so moc 90 phut da hoan thanh

        int completedPeriods =
            item.waitingSeconds
            / THOI_GIAN_TANG;


        // Moc cuoi da xu ly

        int lastPeriod =
            getLastPeriod(
                db,
                item.checkinId
            );


        // Xu ly tung moc chua duoc tang

        for (
            int period =
                lastPeriod + 1;

            period <=
                completedPeriods;

            period++
        )
        {
            currentPriority =
                getCurrentPriority(
                    db,
                    item.checkinId
                );


            // Muc 1 la cao nhat

            if (currentPriority <= 1)
            {
                break;
            }


            increasePriority(
                db,
                item.checkinId,
                period
            );
        }
    }
}


// ========================================
// MAIN
// ========================================

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
        cout
            << "Khong mo duoc priority.db\n";

        return 1;
    }


    cout
        << "========================================\n";

    cout
        << " HE THONG CAP NHAT UU TIEN TU DONG\n";

    cout
        << "========================================\n";

    cout
        << "Moi 90 phut cho hop le: tang 1 cap.\n";

    cout
        << "Nhan S de dung chuong trinh.\n\n";


    while (true)
    {
        // Nhan S de dung

        if (_kbhit())
        {
            char key =
                _getch();


            if (
                key == 's' ||
                key == 'S'
            )
            {
                cout
                    << "\nDa dung chuong trinh.\n";

                break;
            }
        }


        // Chi xu ly trong gio lam viec

        time_t now =
            time(nullptr);


        if (isWorkingTime(now))
        {
            processAutoPriority(db);
        }


        // Sau 10 giay kiem tra lai

        this_thread::sleep_for(
            chrono::seconds(10)
        );
    }


    sqlite3_close(db);


    return 0;
}