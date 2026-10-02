#include <iostream>
#include <sqlite3.h>

using namespace std;

int main()
{
    sqlite3* db = nullptr;

    const char* dbPath =
        "THAY_DOI_MUC_DO_UU_TIEN/db/priority.db";

    //Kiểm tra db có mở được ko 
    if (sqlite3_open(dbPath, &db) != SQLITE_OK)
    {
        cerr << "Khong mo duoc priority.db\n";

        if (db != nullptr)
            sqlite3_close(db);

        return 1;
    }

    //Foreign key
    sqlite3_exec(
        db,
        "PRAGMA foreign_keys = ON;",
        nullptr,
        nullptr,
        nullptr
    );

    //Tạo bảng
    const char* sql = R"(

        CREATE TABLE IF NOT EXISTS priority_checkins
        (
            checkin_id INTEGER PRIMARY KEY,             //id checkin
            patient_id INTEGER NOT NULL,                //id bệnh nhân
            department TEXT NOT NULL,                   //Khoa
            checkin_time TEXT NOT NULL,                 //Thời gian checkin
            base_priority INTEGER NOT NULL              //Mức độ ưu tiên ban đầu
                CHECK(base_priority BETWEEN 1 AND 5),

            current_priority INTEGER NOT NULL           //Mức độ ưu tiên hiện tại
                CHECK(current_priority BETWEEN 1 AND 5),

            last_update TEXT,                           //Thời điểm mức độ ưu tiên được cập nhật gần nhất 

            waiting_seconds INTEGER NOT NULL            //Thời gian đợi cập nhật mức độ ưu tiên
                DEFAULT 0
                CHECK(waiting_seconds >= 0),            

            last_processed_period INTEGER NOT NULL      //Mốc tăng tự động cuối cùng đã xử lí
                DEFAULT 0
                CHECK(last_processed_period >= 0)
        );

    )";

    char* error = nullptr;

    if (sqlite3_exec(
            db,
            sql,
            nullptr,
            nullptr,
            &error
        ) != SQLITE_OK)
    {
        cerr << "Loi tao bang: "
             << error
             << '\n';

        sqlite3_free(error);

        sqlite3_close(db);

        return 1;
    }

    cout << "Tao bang priority_checkins thanh cong.\n";

    sqlite3_close(db);

    return 0;
}