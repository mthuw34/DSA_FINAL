// Ngăn nội dung tệp tiêu đề được nạp lặp lại.
#pragma once

// Lớp giao diện để phần menu yêu cầu xóa bệnh nhân.
class XoaBenhNhan {
public:
    // Yêu cầu xóa các lượt khám thuộc mã bệnh nhân được truyền vào.
    static bool xoaBenhNhan(int patientId);
    // Yêu cầu xóa toàn bộ bệnh nhân khỏi các cơ sở dữ liệu của chức năng.
    static bool xoaTatCaBenhNhan();
};

// Lớp thực hiện thao tác xóa ở tầng dữ liệu.
class DBXoaBenhNhan {
public:
    // Xóa bệnh nhân theo mã khỏi dữ liệu nguồn, sau đó cập nhật hàng đợi.
    static bool xoaBenhNhan(int patientId);
    // Ghi danh sách rỗng vào dữ liệu nguồn và các bảng hàng đợi.
    static bool xoaTatCaBenhNhan();
};