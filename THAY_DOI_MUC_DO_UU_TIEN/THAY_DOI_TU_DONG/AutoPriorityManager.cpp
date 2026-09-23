#include "AutoPriorityManager.h"

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

using namespace std;


// ========================================
// DOI CHUOI THOI GIAN SANG TIMESTAMP
// ========================================

long long convertToTimestamp(
    const string& timeString
)
{
    tm timeInfo = {};

    stringstream ss(timeString);

    ss >> get_time(
        &timeInfo,
        "%Y-%m-%d %H:%M:%S"
    );

    if (ss.fail())
    {
        return 0;
    }

    return static_cast<long long>(
        mktime(&timeInfo)
    );
}


// ========================================
// CONSTRUCTOR
// ========================================

AutoPriorityManager::AutoPriorityManager(
    sqlite3* database
)
{
    db = database;
}


// ========================================
// DOC CAC BENH NHAN CHUA DUOC AUTO UPDATE
// VA DUA VAO MIN-HEAP
// ========================================

bool AutoPriorityManager::loadWaitingPatients()
{
    const char* sql = R"(

        SELECT
            checkin_id,
            patient_id,
            checkin_time,
            current_priority

        FROM priority_checkins

        WHERE auto_boosted = 0;

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
        cerr << "Loi doc priority.db: "
             << sqlite3_errmsg(db)
             << '\n';

        return false;
    }

    while (
        sqlite3_step(stmt)
        ==
        SQLITE_ROW
    )
    {
        AutoPriorityItem item;

        // =========================
        // CHECKIN ID
        // =========================

        item.checkinId =
            sqlite3_column_int(
                stmt,
                0
            );


        // =========================
        // PATIENT ID
        // =========================

        item.patientId =
            sqlite3_column_int(
                stmt,
                1
            );


        // =========================
        // CHECKIN TIME
        // =========================

        const unsigned char* text =
            sqlite3_column_text(
                stmt,
                2
            );

        if (text != nullptr)
        {
            item.checkinTime =
                reinterpret_cast<const char*>(
                    text
                );
        }
        else
        {
            item.checkinTime = "";
        }


        // =========================
        // PRIORITY HIEN TAI
        // =========================

        item.currentPriority =
            sqlite3_column_int(
                stmt,
                3
            );


        // =========================
        // TINH THOI DIEM DU 1 GIO
        // =========================

        long long checkinTimestamp =
            convertToTimestamp(
                item.checkinTime
            );

        if (checkinTimestamp == 0)
        {
            cout
                << "Khong doc duoc thoi gian cua checkin_id = "
                << item.checkinId
                << '\n';

            continue;
        }

        // 1 gio = 3600 giay

        item.dueTime =
            checkinTimestamp + 3600;


        // =========================
        // DUA VAO MIN-HEAP
        // =========================

        heap.insert(item);
    }

    sqlite3_finalize(stmt);

    cout
        << "Da nap "
        << heap.getSize()
        << " benh nhan vao Min-Heap.\n";

    return true;
}


// ========================================
// HIEN BENH NHAN DEN HAN SOM NHAT
// ========================================

void AutoPriorityManager::showNextPatient()
{
    if (heap.isEmpty())
    {
        cout
            << "Khong co benh nhan nao trong Heap.\n";

        return;
    }

    AutoPriorityItem item =
        heap.peek();

    cout << "\n===== BENH NHAN DEN HAN SOM NHAT =====\n";

    cout
        << "Checkin ID: "
        << item.checkinId
        << '\n';

    cout
        << "Patient ID: "
        << item.patientId
        << '\n';

    cout
        << "Check-in time: "
        << item.checkinTime
        << '\n';

    cout
        << "Priority hien tai: "
        << item.currentPriority
        << '\n';


    time_t due =
        static_cast<time_t>(
            item.dueTime
        );

    tm* dueInfo =
        localtime(&due);

    cout << "Du 1 gio luc: ";

    if (dueInfo != nullptr)
    {
        cout
            << put_time(
                dueInfo,
                "%Y-%m-%d %H:%M:%S"
            );
    }

    cout << '\n';
}