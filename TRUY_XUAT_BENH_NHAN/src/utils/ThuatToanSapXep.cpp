#include "ThuatToanSapXep.h"

using namespace std;

int ThuatToanSapXep::layThuTuKhoa(const string& department) {
    static const vector<string> departments = {
        "Khoa Cap cuu", "Khoa Noi", "Khoa Ngoai", "Khoa Tim mach",
        "Khoa Nhi", "Khoa San", "Khoa Tai Mui Hong", "Khoa Mat",
        "Khoa Da lieu", "Khoa Than kinh"
    };

    for (int index = 0; index < static_cast<int>(departments.size()); ++index) {
        if (departments[index] == department) {
            return index + 1;
        }
    }
    return 99;
}   

bool ThuatToanSapXep::xetUuTien(const HoSoTruyXuat& left, const HoSoTruyXuat& right) {
    if (left.departmentOrder != right.departmentOrder) {
        return left.departmentOrder < right.departmentOrder;
    }
    
    if (left.currentPriority != right.currentPriority) {
        return left.currentPriority < right.currentPriority;
    }
    
    // ĐOẠN MỚI THÊM: Ưu tiên ca trở nặng thật sự (do Y tá bấm)
    if (left.isTroNangLamSang != right.isTroNangLamSang) {
        return left.isTroNangLamSang > right.isTroNangLamSang;
    }
    
    // Ưu tiên lùi về bệnh gốc (Dành cho nhóm tự động được lên cấp do đợi lâu)
    if (left.basePriority != right.basePriority) {
        return left.basePriority < right.basePriority;
    }
    
    if (left.lastUpdate != right.lastUpdate) {
        return left.lastUpdate < right.lastUpdate;
    }
    
    if (left.checkinTime != right.checkinTime) {
        return left.checkinTime < right.checkinTime;
    }
    
    return left.checkinId < right.checkinId;
}

void ThuatToanSapXep::tron(vector<HoSoTruyXuat>& records, vector<HoSoTruyXuat>& buffer, int left, int middle, int right) {
    int first = left;
    int second = middle + 1;
    int target = left;

    while (first <= middle && second <= right) {
        if (xetUuTien(records[first], records[second])) {
            buffer[target++] = records[first++];
        } else {
            buffer[target++] = records[second++];
        }
    }

    while (first <= middle) {
        buffer[target++] = records[first++];
    }
    while (second <= right) {
        buffer[target++] = records[second++];
    }

    for (int index = left; index <= right; ++index) {
        records[index] = buffer[index];
    }
}

void ThuatToanSapXep::sapXepTron(vector<HoSoTruyXuat>& records, vector<HoSoTruyXuat>& buffer, int left, int right) {
    if (left >= right) {
        return;
    }

    const int middle = left + (right - left) / 2;
    sapXepTron(records, buffer, left, middle);
    sapXepTron(records, buffer, middle + 1, right);
    tron(records, buffer, left, middle, right);
}