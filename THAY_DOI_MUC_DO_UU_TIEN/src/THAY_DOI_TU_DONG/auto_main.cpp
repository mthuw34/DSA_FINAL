#include <iostream>
#include <thread>
#include <chrono>
#include <conio.h>
#include <filesystem>
#include <sqlite3.h>

#include "AutoPriority.h"
#include "PrioritySync.h"

int main()
{
    // Mở các database.
    sqlite3* hospitalDb = nullptr;
    sqlite3* priorityDb = nullptr;
    sqlite3* examsDb = nullptr;

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
        std::cout << "Khong mo duoc hospital.db\n";

        if (hospitalDb != nullptr)
        {
            std::cout << sqlite3_errmsg(hospitalDb) << '\n';
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
        std::cout << "Khong mo duoc priority.db\n";

        if (priorityDb != nullptr)
        {
            std::cout << sqlite3_errmsg(priorityDb) << '\n';
            sqlite3_close(priorityDb);
        }

        sqlite3_close(hospitalDb);
        return 1;
    }

    if (std::filesystem::exists("DANG_KHAM/db/dangKham.db") &&
        sqlite3_open_v2("DANG_KHAM/db/dangKham.db", &examsDb, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
    {
        std::cout << "Khong mo duoc dangKham.db\n";
        if (examsDb) sqlite3_close(examsDb);
        sqlite3_close(priorityDb);
        sqlite3_close(hospitalDb);
        return 1;
    }

    // Khởi tạo đối tượng đồng bộ.
    PrioritySync sync(hospitalDb, priorityDb, examsDb);

    std::cout << "====================================\n";
    std::cout << " CAP NHAT UU TIEN TU DONG\n";
    std::cout << "====================================\n";
    std::cout << "Tu dong dong bo tu hospital.db sang priority.db.\n";
    std::cout << "Check-in moi: tinh cho tu thoi gian check-in.\n";
    std::cout << "Moi 90 phut cho trong gio lam: tang 1 muc.\n";
    std::cout << "Tinh bu cac moc da qua, dung o muc 1.\n";
    std::cout << "Khong tinh thoi gian nghi trua va ngoai gio.\n";
    std::cout << "Kiem tra lai sau moi 30 giay.\n";
    std::cout << "Nhan S de dung.\n\n";

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

        // Đồng bộ trước khi cập nhật ưu tiên.
        if (sync.syncAll())
        {
            // Nạp dữ liệu mới nhất vào heap.
            AutoPriorityHeap heap;
            loadPatients(priorityDb, heap);

            // Xử lý các mốc tăng ưu tiên đã đến.
            processAuto(priorityDb, heap);
        }
        else
        {
            std::cout << "Dong bo that bai. Se thu lai o luot sau.\n";
        }

        // Chờ 30 giây, kiểm tra phím S mỗi 100 mili giây.
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

            std::this_thread::sleep_for(
                std::chrono::milliseconds(100)
            );
        }
    }

    // Đóng các kết nối database.
    if (examsDb) sqlite3_close(examsDb);
    sqlite3_close(priorityDb);
    sqlite3_close(hospitalDb);

    std::cout << "\nDa dung chuong trinh.\n";
    return 0;
}