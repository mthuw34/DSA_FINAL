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
#include "PrioritySync.h"
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
        time_t updateTime = item.nextBoostTime;

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
    sqlite3* hospitalDb = nullptr;
    sqlite3* priorityDb = nullptr;

    const char* hospitalPath =
        "QUAN_LY_BENH_NHAN/db/hospital.db";

    const char* priorityPath =
        "THAY_DOI_MUC_DO_UU_TIEN/db/priority.db";

    // Mở database gốc để đọc dữ liệu.
    if (sqlite3_open_v2(
            hospitalPath,
            &hospitalDb,
            SQLITE_OPEN_READONLY,
            nullptr
        ) != SQLITE_OK)
    {
        cout << "Khong mo duoc hospital.db\n";

        if (hospitalDb != nullptr)
        {
            sqlite3_close(hospitalDb);
        }

        return 1;
    }

    // Mở database ưu tiên để đọc và cập nhật.
    if (sqlite3_open_v2(
            priorityPath,
            &priorityDb,
            SQLITE_OPEN_READWRITE,
            nullptr
        ) != SQLITE_OK)
    {
        cout << "Khong mo duoc priority.db\n";

        if (priorityDb != nullptr)
        {
            sqlite3_close(priorityDb);
        }

        sqlite3_close(hospitalDb);
        return 1;
    }

    // Khởi tạo đối tượng đồng bộ giữa hai database.
    PrioritySync sync(hospitalDb, priorityDb);

    cout << "====================================\n";
    cout << " CAP NHAT UU TIEN TU DONG\n";
    cout << "====================================\n";
    cout << "Tu dong dong bo tu hospital.db sang priority.db.\n";
    cout << "Check-in moi: tinh cho tu thoi gian check-in.\n";
    cout << "Moi 90 phut cho trong gio lam: tang 1 muc.\n";
    cout << "Tinh bu cac moc da qua, dung o muc 1.\n";
    cout << "Khong tinh thoi gian nghi trua va ngoai gio.\n";
    cout << "Kiem tra lai sau moi 30 giay.\n";
    cout << "Nhan S de dung.\n\n";

    bool running = true;

    while (running)
    {
        // Kiểm tra phím S trước khi xử lý.
        if (_kbhit())
        {
            char key = _getch();

            if (key == 's' || key == 'S')
            {
                break;
            }
        }

        // Đồng bộ check-in mới và các thay đổi từ database gốc.
        if (sync.syncAll())
        {
            // Tạo heap mới để nhận trạng thái dữ liệu mới nhất.
            AutoPriorityHeap heap;
            loadPatients(priorityDb, heap);

            // Xử lý các mốc đã đến, kể cả khi mở ngoài giờ.
            // calculateNextBoostTime chỉ cộng thời gian làm việc.
            processAuto(priorityDb, heap);
        }
        else
        {
            cout << "Dong bo that bai. Se thu lai o luot sau.\n";
        }

        // Chờ 30 giây và kiểm tra phím S trong lúc chờ.
        for (int i = 0; i < 300 && running; i++)
        {
            if (_kbhit())
            {
                char key = _getch();

                if (key == 's' || key == 'S')
                {
                    running = false;
                    break;
                }
            }

            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }

    // Đóng các kết nối database.
    sqlite3_close(priorityDb);
    sqlite3_close(hospitalDb);

    cout << "\nDa dung chuong trinh.\n";
    return 0;
}