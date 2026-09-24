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
    // 1. Phân theo khoa trước
    if (left.departmentOrder != right.departmentOrder) {
        return left.departmentOrder < right.departmentOrder;
    }
    
    // 2. Xét mức độ ưu tiên hiện tại (Current Priority)
    if (left.currentPriority != right.currentPriority) {
        return left.currentPriority < right.currentPriority;
    }
    
    // 3. Ưu tiên lùi về gốc (Base Priority Fallback)
    // Nếu bị trùng currentPriority, ai có bệnh gốc nặng hơn (số nhỏ hơn) lập tức xếp trên!
    if (left.basePriority != right.basePriority) {
        return left.basePriority < right.basePriority;
    }
    
    // 4. Nếu giống nhau cả bệnh gốc lẫn hiện tại, xét thời gian cập nhật tình trạng
    if (left.lastUpdate != right.lastUpdate) {
        return left.lastUpdate < right.lastUpdate;
    }
    
    // 5. Cuối cùng, nguyên tắc đến trước vào trước (FIFO)
    if (left.checkinTime != right.checkinTime) {
        return left.checkinTime < right.checkinTime;
    }
    
    // 6. Chốt chặn cuối bằng ID bốc số
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