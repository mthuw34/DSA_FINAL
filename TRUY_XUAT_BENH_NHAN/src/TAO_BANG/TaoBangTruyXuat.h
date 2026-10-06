// Ngăn nội dung tệp tiêu đề được nạp lặp lại.
#pragma once

// Khai báo thao tác tạo các bảng hàng đợi truy xuất.
class DBTaoBang {
public:
    // Tạo bảng cho từng khoa; trả true nếu toàn bộ giao dịch thành công.
    static bool taoBangTruyXuat();
};