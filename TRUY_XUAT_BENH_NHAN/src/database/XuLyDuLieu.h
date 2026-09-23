#pragma once
#include <sqlite3.h>
#include <vector>
#include "HoSoTruyXuat.h"
using namespace std;

class XuLyDuLieu {
public:
    static bool layDanhSachBenhNhan(
        sqlite3* hospitalDatabase,
        sqlite3* priorityDatabase,
        vector<HoSoTruyXuat>& outRecords
    );
    static bool xuatDuLieuDaSapXep(const vector<HoSoTruyXuat>& sortedRecords, const char* outputPath);
};