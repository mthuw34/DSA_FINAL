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

// Chuyển đổi thời gian thành time_t
bool parseTime(const string& text, time_t& result)
{
    tm t = {};
    stringstream ss(text);

    ss >> get_time(&t, "%Y-%m-%d %H:%M:%S");

    if (ss.fail())
    {
        return false;
    }

    t.tm_isdst = -1;
    result = mktime(&t);
    return result != static_cast<time_t>(-1);
}

// Nạp tất cả các bệnh nhân vào Heap
void loadPatients(sqlite3* db, AutoPriorityHeap& heap)
{
    const char* sql = R"(

        SELECT
            checkin_id,
            current_priority,
            last_update

        FROM priority_checkins;

    )";

    sqlite3_stmt* stmt = nullptr;

    if (
        sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK
    )
    {
        cout << "Loi doc priority.db\n";
        return;
    }

    //Duyệt từng bệnh nhân
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int checkinId = sqlite3_column_int(stmt, 0);
        int priority = sqlite3_column_int(stmt, 1);

        // Mức ưu tiên 1 không tăng
        if (priority <= 1)
        {
            continue;
        }

        const unsigned char* text = sqlite3_column_text(stmt, 2);
        if (text == nullptr)
        {
            continue;
        }

        string lastUpdate = reinterpret_cast<const char*>(text);
        time_t startTime;

        if (!parseTime(lastUpdate, startTime))
        {
            continue;
        }

        AutoPriorityItem item;

        item.checkinId = checkinId;
        item.nextBoostTime = calculateNextBoostTime(startTime);

        heap.insert(item);
    }

    sqlite3_finalize(stmt);
}

// Đọc mức độ ưu tiên hiện tại từ database 
bool getPriority(
    sqlite3* db,
    int checkinId,
    int& priority,
    string& lastUpdate
)
{
    const char* sql = R"(

        SELECT
            current_priority,
            last_update

        FROM priority_checkins

        WHERE checkin_id = ?;

    )";

    sqlite3_stmt* stmt = nullptr;

    if (
        sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK
    )
    {
        return false;
    }


    sqlite3_bind_int(stmt, 1, checkinId);

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return false;
    }

    priority = sqlite3_column_int(stmt, 0);

    const unsigned char* text = sqlite3_column_text(stmt, 1);

    lastUpdate = text ? reinterpret_cast<const char*>(text): "";

    sqlite3_finalize(stmt);

    return true;
}

// Cập nhật mức đọ ưu tiên tự động
bool updatePriority(sqlite3* db, int checkinId, int newPriority, time_t updateTime)
{
    tm info = *localtime(&updateTime);
    char timeText[20];

    strftime(
        timeText,
        sizeof(timeText),
        "%Y-%m-%d %H:%M:%S",
        &info
    );


    const char* sql = R"(

        UPDATE priority_checkins

        SET
            current_priority = ?,
            last_update = ?

        WHERE checkin_id = ?;

    )";

    sqlite3_stmt* stmt = nullptr;


    if (
        sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &stmt,
            nullptr
        ) != SQLITE_OK
    )
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, newPriority);

    sqlite3_bind_text(
        stmt,
        2,
        timeText,
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(stmt, 3, checkinId);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;

    sqlite3_finalize(stmt);

    return success;
}

// Xử lí các bệnh nhân đã đến mốc
void processAuto(sqlite3* db, AutoPriorityHeap& heap)
{
    time_t now = time(nullptr);

    AutoPriorityItem item;

    while (heap.peek(item) && item.nextBoostTime <= now)
    {
        heap.extractMin(item);
        int currentPriority;
        string lastUpdate;

        if (!getPriority(db, item.checkinId, currentPriority, lastUpdate))
        {
            continue;
        }

        // Đã ở mức cao nhất
        if (currentPriority <= 1)
        {
            continue;
        }

        time_t lastUpdateTime;

        if (!parseTime(lastUpdate, lastUpdateTime))
        {
            continue;
        }

        time_t correctBoostTime = calculateNextBoostTime(lastUpdateTime);

        // Nếu bác sĩ cập nhật thủ công, mốc trong heap cũ không còn dùng
        if (item.nextBoostTime != correctBoostTime)
        {
            item.nextBoostTime = correctBoostTime;
            heap.insert(item);
            continue;
        }

        int newPriority = currentPriority - 1;
        time_t updateTime = time(nullptr);

        if (updatePriority(db, item.checkinId, newPriority, updateTime ) )
        {
            cout<< "Checkin ID "
                << item.checkinId
                << ": "
                << currentPriority
                << " -> "
                << newPriority
                << '\n';

            // Chưa đếm mức 1 thì đưa lại vào heap
            if (newPriority > 1)
            {
                AutoPriorityItem newItem;
                newItem.checkinId = item.checkinId;
                newItem.nextBoostTime = calculateNextBoostTime(updateTime);
                heap.insert(newItem);
            }
        }
    }
}

int main()
{
    sqlite3* db = nullptr;
    const char* dbPath = "THAY_DOI_MUC_DO_UU_TIEN/db/priority.db";

    //Mở database priority
    if (
        sqlite3_open_v2(
            dbPath,
            &db,
            SQLITE_OPEN_READWRITE,
            nullptr
        ) != SQLITE_OK
    )
    {
        cout << "Khong mo duoc priority.db\n";
        return 1;
    }

    //Tạo heap
    AutoPriorityHeap heap;
    loadPatients(db, heap);

    cout << "====================================\n";
    cout << " CAP NHAT UU TIEN TU DONG\n";
    cout << "====================================\n";
    cout << "So benh nhan trong Heap: " << heap.getSize() << '\n';
    cout << "Moi 90 phut cho hop le: tang 1 muc.\n";
    cout << "Nhan S de dung.\n\n";

    //Vòng lặp cập nhật tự động
    while (true)
    {
        if (_kbhit())
        {
            char key =_getch();
            //Nhấn 's' để dừng chương trình
            if (key == 's' || key == 'S')
            {
                cout << "\nDa dung chuong trinh.\n";
                break;
            }
        }

        time_t now = time(nullptr);

        if (isWorkingTime(now))
        {
            processAuto(db, heap);
        }
        this_thread::sleep_for(chrono::seconds(10));
    }

    sqlite3_close(db);

    return 0;
}