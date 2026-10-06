// Nạp khai báo thao tác tạo bảng.
#include "TaoBangTruyXuat.h"

// Điểm bắt đầu riêng của chương trình khởi tạo các bảng truy xuất.
int main() {
    // Gọi hàm tạo bảng; quy đổi true/false thành mã thoát thành công/thất bại.
    return DBTaoBang::taoBangTruyXuat() ? 0 : 1;
}
