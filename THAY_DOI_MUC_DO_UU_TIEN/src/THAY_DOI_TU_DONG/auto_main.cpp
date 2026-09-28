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
#include "AutoPriority.h"

using namespace std;


// ========================================
// 1 MOC = 90 PHUT = 5400 GIAY
// ========================================

const long long THOI_GIAN_TANG =
    90 * 60;


// ========================================
// DOI CHUOI THOI GIAN -> time_t
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
    {
        return false;
    }


    t.tm_isdst = -1;


    result = mktime(&t);


    return
        result
        !=
        static_cast<time_t>(-1);
}


// ========================================
// CAP NHAT THOI GIAN CHO
// ========================================

void updateWaitingTime(
    sqlite3* db
)
{
    const char* selectSql = R"(

        SELECT
            checkin_id,
            checkin_time

        FROM priority_checkins;

    )";


    sqlite3_stmt* selectStmt =
        nullptr;


    if (sqlite3_prepare_v2(
            db,
            selectSql,
            -1,
            &selectStmt,
            nullptr
        ) != SQLITE_OK)
    {
        return;
    }


    const char* updateSql = R"(

        UPDATE priority_checkins

        SET waiting_seconds = ?

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
        sqlite3_finalize(selectStmt);

        return;
    }


    time_t now =
        time(nullptr);


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
        {
            continue;
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


        sqlite3_bind_int64(
            updateStmt,
            1,
            waitingSeconds
        );


        sqlite3_bind_int(
            updateStmt,
            2,
            checkinId
        );


        sqlite3_step(updateStmt);
    }


    sqlite3_finalize(selectStmt);

    sqlite3_finalize(updateStmt);
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
            checkin_id,
            waiting_seconds,
            last_processed_period

        FROM priority_checkins

        WHERE current_priority > 1;

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
        AutoPriorityItem item;


        item.checkinId =
            sqlite3_column_int(
                stmt,
                0
            );


        item.waitingSeconds =
            sqlite3_column_int64(
                stmt,
                1
            );


        int lastProcessed =
            sqlite3_column_int(
                stmt,
                2
            );


        item.nextPeriod =
            lastProcessed + 1;


        long long nextThreshold =
            static_cast<long long>(
                item.nextPeriod
            )
            * THOI_GIAN_TANG;


        item.remainingSeconds =
            nextThreshold
            -
            item.waitingSeconds;


        heap.insert(item);
    }


    sqlite3_finalize(stmt);
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
// TANG 1 CAP UU TIEN
// ========================================

bool increasePriority(
    sqlite3* db,
    int checkinId,
    int period
)
{
    const char* sql = R"(

        UPDATE priority_checkins

        SET
            current_priority =
                current_priority - 1,

            last_processed_period = ?,

            last_update =
                datetime(
                    'now',
                    'localtime'
                )

        WHERE
            checkin_id = ?

            AND current_priority > 1;

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
        period
    );


    sqlite3_bind_int(
        stmt,
        2,
        checkinId
    );


    bool success =
        sqlite3_step(stmt)
        == SQLITE_DONE;


    sqlite3_finalize(stmt);


    return success;
}


// ========================================
// XU LY TU DONG
// ========================================

void processAuto(
    sqlite3* db
)
{
    // Tinh lai tong thoi gian cho

    updateWaitingTime(db);


    // Tao Min-Heap

    AutoPriorityHeap heap;


    loadHeap(
        db,
        heap
    );


    AutoPriorityItem item;


    // ====================================
    // LAY BENH NHAN DEN HAN SOM NHAT
    // ====================================

    while (
        heap.peek(item)
    )
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


        if (
            currentPriority <= 1
        )
        {
            continue;
        }


        // Tong so moc 90 phut
        // benh nhan da hoan thanh

        int completedPeriods =
            static_cast<int>(
                item.waitingSeconds
                /
                THOI_GIAN_TANG
            );


        int lastProcessed =
            item.nextPeriod - 1;


        // ====================================
        // XU LY CAC MOC CHUA CAP NHAT
        // ====================================

        for (
            int period =
                lastProcessed + 1;

            period <=
                completedPeriods;

            period++
        )
        {
            int oldPriority =
                getCurrentPriority(
                    db,
                    item.checkinId
                );


            // Muc 1 la cao nhat

            if (oldPriority <= 1)
            {
                break;
            }


            if (
                increasePriority(
                    db,
                    item.checkinId,
                    period
                )
            )
            {
                int newPriority =
                    getCurrentPriority(
                        db,
                        item.checkinId
                    );


                cout
                    << "Checkin ID "
                    << item.checkinId
                    << ": "
                    << oldPriority
                    << " -> "
                    << newPriority
                    << '\n';
            }
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
        << "Moi 90 phut cho hop le: tang 1 muc do uu tien.\n";

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
                key == 's'
                ||
                key == 'S'
            )
            {
                cout
                    << "\nDa dung chuong trinh.\n";

                break;
            }
        }


        time_t now =
            time(nullptr);


        // Chi tang priority trong gio lam viec

        if (
            isWorkingTime(now)
        )
        {
            processAuto(db);
        }


        // 10 giay kiem tra lai

        this_thread::sleep_for(
            chrono::seconds(10)
        );
    }


    sqlite3_close(db);


    return 0;
}