// Nạp khai báo giao diện xóa và lớp xử lý dữ liệu phía dưới.
#include "XoaBenhNhan.h"

// Chuyển yêu cầu xóa một bệnh nhân tới lớp xử lý dữ liệu.
bool XoaBenhNhan::xoaBenhNhan(int patientId) {
    // Trả nguyên trạng thái thành công/thất bại từ tầng xử lý dữ liệu.
    return DBXoaBenhNhan::xoaBenhNhan(patientId);
}

// Chuyển yêu cầu xóa toàn bộ bệnh nhân tới lớp xử lý dữ liệu.
bool XoaBenhNhan::xoaTatCaBenhNhan() {
    return DBXoaBenhNhan::xoaTatCaBenhNhan();
}
