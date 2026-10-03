#include <iostream>
#include <thread>
#include <chrono>
#include <conio.h>
#include <sqlite3.h>

#include "AutoPriority.h"
#include "PrioritySync.h"
using namespace std;

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