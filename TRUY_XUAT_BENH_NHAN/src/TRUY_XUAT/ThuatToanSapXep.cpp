#include "TruyXuat.h"
#include "MangDongBenhNhan.h"

using namespace std;

int ThuatToanSapXep::layThuTuKhoa(const string& department) {
    return CauHinhTruyXuat::thuTuKhoa(department);
}   

bool ThuatToanSapXep::xetUuTien(const HoSoTruyXuat& left, const HoSoTruyXuat& right) {
    // Smaller numbers mean higher priority (1 is the highest).
    if (left.currentPriority != right.currentPriority) {
        return left.currentPriority < right.currentPriority;
    }

    // Equal priorities follow check-in time, regardless of how priority changed.
    // YYYY-MM-DD HH:MM:SS also sorts correctly across different dates.
    if (left.checkinTime != right.checkinTime) {
        return left.checkinTime < right.checkinTime;
    }

    // IDs preserve intake order for check-ins recorded in the same second.
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