#pragma once
#include "BenhNhanKham.h"
#include <sqlite3.h>
#include <string>
#include <vector>

class XuLyDatabase {
public:
    ~XuLyDatabase();

    bool MoDatabase(const std::string& duongDan);
    void DongDatabase();

    // Nạp bệnh nhân của 10 khoa
    bool LayTatCaBenhNhan(std::vector<BenhNhanKham>& danhSach);


    sqlite3* LayDatabase() const;

private:
    sqlite3* Database = nullptr;
};
