#pragma once
#include <vector>
#include <string>
#include "HoSoTruyXuat.h"
using namespace std;

namespace ThuatToanSapXep {
    int layThuTuKhoa(const string& department);
    bool xetUuTien(const HoSoTruyXuat& left, const HoSoTruyXuat& right);
    void tron(vector<HoSoTruyXuat>& records, vector<HoSoTruyXuat>& buffer, int left, int middle, int right);
    void sapXepTron(vector<HoSoTruyXuat>& records, vector<HoSoTruyXuat>& buffer, int left, int right);
}