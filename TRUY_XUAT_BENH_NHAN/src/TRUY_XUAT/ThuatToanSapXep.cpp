#include "TruyXuat.h"
#include "MangDongBenhNhan.h"

using namespace std;

int ThuatToanSapXep::layThuTuKhoa(const string& department) {
    return CauHinhTruyXuat::thuTuKhoa(department);
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
    
    // 3. Ưu tiên ca trở nặng thật sự (do Y tá bấm)
    if (left.isTroNangLamSang != right.isTroNangLamSang) {
        return left.isTroNangLamSang > right.isTroNangLamSang;
    }
    
    // 4. Ưu tiên lùi về bệnh gốc (Dành cho nhóm tự động được lên cấp do đợi lâu)
    if (left.basePriority != right.basePriority) {
        return left.basePriority < right.basePriority;
    }
    
    // 5. Nếu cùng mức ưu tiên, xét ngày + giờ check-in để giữ đúng FIFO qua nhiều ngày.
    // Chuỗi thời gian có dạng YYYY-MM-DD HH:MM:SS nên so sánh từ điển cũng đúng thứ tự thời gian.
    if (left.checkinTime != right.checkinTime) {
        return left.checkinTime < right.checkinTime;
    }

    // 6. Nếu cùng cả thời điểm check-in, dùng lần cập nhật gần nhất làm khóa phụ.
    if (left.lastUpdate != right.lastUpdate) {
        return left.lastUpdate < right.lastUpdate;
    }
    
    // checkinId provides deterministic ordering when all clinical/time keys match;
    // it is a tie-breaker, not by itself proof of stability relative to input order.
    return left.checkinId < right.checkinId;
}

void ThuatToanSapXep::tron(MangDongBenhNhan& records, MangDongBenhNhan& buffer, int left, int middle, int right) {
    // Precondition: 0 <= left <= middle < right < records.size(), and buffer.size() >= records.size().
    int first = left;
    int second = middle + 1;
    int target = left;

    while (first <= middle && second <= right) {
        // Pick the left item unless the right one strictly precedes it; this preserves
        // input order for comparator-equivalent items. Selecting right on equality breaks stability.
        if (!xetUuTien(records[second], records[first])) {
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

void ThuatToanSapXep::sapXepTron(MangDongBenhNhan& records, MangDongBenhNhan& buffer, int left, int right) {
    // Precondition: buffer.size() >= records.size(), and [left, right] is a valid range or empty.
    if (left >= right) {
        return;
    }

    const int middle = left + (right - left) / 2;
    sapXepTron(records, buffer, left, middle);
    sapXepTron(records, buffer, middle + 1, right);
    tron(records, buffer, left, middle, right);
}