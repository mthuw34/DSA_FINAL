// Nạp các khai báo cấu hình khoa, hồ sơ và thuật toán sắp xếp.
#include "TruyXuat.h"
// Nạp các thao tác truy cập mảng động chứa hồ sơ.
#include "MangDongBenhNhan.h"

// Cho phép dùng tên kiểu/tiện ích chuẩn mà không cần tiền tố std::.
using namespace std;

// Lấy thứ tự khoa từ cấu hình dùng chung.
int ThuatToanSapXep::layThuTuKhoa(const string& department) {
    return CauHinhTruyXuat::thuTuKhoa(department);
}   

// Trả true khi left phải được xếp trước right theo quy tắc ưu tiên.
bool ThuatToanSapXep::xetUuTien(const HoSoTruyXuat& left, const HoSoTruyXuat& right) {
<<<<<<< HEAD
    // Smaller numbers mean higher priority (1 is the highest).
    if (left.currentPriority != right.currentPriority) {
        return left.currentPriority < right.currentPriority;
    }

    // Equal priorities follow check-in time, regardless of how priority changed.
    // YYYY-MM-DD HH:MM:SS also sorts correctly across different dates.
=======
    // Ưu tiên nhóm theo khoa; khoa có số thứ tự nhỏ hơn được xếp trước.
    if (left.departmentOrder != right.departmentOrder) {
        return left.departmentOrder < right.departmentOrder;
    }
    
    // Trong cùng khoa, mức ưu tiên hiện tại có giá trị nhỏ hơn được xếp trước.
    if (left.currentPriority != right.currentPriority) {
        return left.currentPriority < right.currentPriority;
    }
    
    // Nếu cờ trở nặng khác nhau, hồ sơ được đánh dấu trở nặng xếp trước.
    if (left.isTroNangLamSang != right.isTroNangLamSang) {
        return left.isTroNangLamSang > right.isTroNangLamSang;
    }
    
    // Nếu vẫn hòa, xét mức ưu tiên ban đầu theo thứ tự tăng dần.
    if (left.basePriority != right.basePriority) {
        return left.basePriority < right.basePriority;
    }
    
    // Xét thời điểm đăng ký tăng dần để lượt đến trước đứng trước (FIFO).
    // So sánh chuỗi đúng thứ tự thời gian khi định dạng là YYYY-MM-DD HH:MM:SS.
>>>>>>> 83064261dd8b1778bff88c316d0eea7c58b646d0
    if (left.checkinTime != right.checkinTime) {
        return left.checkinTime < right.checkinTime;
    }

<<<<<<< HEAD
    // IDs preserve intake order for check-ins recorded in the same second.
=======
    // Dùng thời điểm cập nhật làm khóa phụ khi thời điểm đăng ký trùng nhau.
    if (left.lastUpdate != right.lastUpdate) {
        return left.lastUpdate < right.lastUpdate;
    }
    
    // Dùng mã lượt khám để tạo thứ tự xác định khi các tiêu chí phía trên đều hòa.
>>>>>>> 83064261dd8b1778bff88c316d0eea7c58b646d0
    return left.checkinId < right.checkinId;
}

// Ghép hai đoạn đã sắp xếp thành một đoạn theo cùng quy tắc ưu tiên.
void ThuatToanSapXep::tron(MangDongBenhNhan& records, MangDongBenhNhan& buffer, int left, int middle, int right) {
    // Điều kiện: các chỉ số nằm trong records và buffer đủ chỗ chứa toàn bộ records.
    // first duyệt nửa trái, second duyệt nửa phải, target ghi vào buffer.
    int first = left;
    int second = middle + 1;
    int target = left;

    // So sánh hai phần tử đầu chưa ghép và lấy phần tử đứng trước.
    while (first <= middle && second <= right) {
        // Nếu phần tử bên phải không đứng trước phần tử bên trái, lấy bên trái trước.
        // Cách chọn này giữ thứ tự tương đối của các phần tử có mức ưu tiên tương đương.
        if (!xetUuTien(records[second], records[first])) {
            buffer[target++] = records[first++];
        } else {
            buffer[target++] = records[second++];
        }
    }

    // Chép nốt các phần tử còn lại của nửa trái, nếu nửa này chưa hết.
    while (first <= middle) {
        buffer[target++] = records[first++];
    }
    // Chép nốt các phần tử còn lại của nửa phải, nếu nửa này chưa hết.
    while (second <= right) {
        buffer[target++] = records[second++];
    }

    // Chép đoạn đã ghép từ bộ đệm trở lại mảng dữ liệu chính.
    for (int index = left; index <= right; ++index) {
        records[index] = buffer[index];
    }
}

// Sắp xếp đoạn [left, right] bằng cách chia đôi rồi ghép các đoạn đã sắp xếp.
void ThuatToanSapXep::sapXepTron(MangDongBenhNhan& records, MangDongBenhNhan& buffer, int left, int right) {
    // Điều kiện: buffer đủ chỗ và đoạn chỉ số hợp lệ hoặc rỗng.
    // Đoạn một phần tử hoặc rỗng đã có thứ tự nên là điểm dừng đệ quy.
    if (left >= right) {
        return;
    }

    // Tính điểm giữa để chia đoạn thành hai nửa mà không cộng trực tiếp left + right.
    const int middle = left + (right - left) / 2;
    // Sắp xếp đệ quy nửa trái.
    sapXepTron(records, buffer, left, middle);
    // Sắp xếp đệ quy nửa phải.
    sapXepTron(records, buffer, middle + 1, right);
    // Ghép hai nửa đã sắp xếp thành một đoạn có thứ tự.
    tron(records, buffer, left, middle, right);
}