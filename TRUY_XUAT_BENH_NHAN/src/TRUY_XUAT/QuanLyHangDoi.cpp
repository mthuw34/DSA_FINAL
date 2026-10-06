// Nạp khai báo lớp quản lý, thao tác cơ sở dữ liệu và thuật toán sắp xếp.
#include "TruyXuat.h"
// Nạp định nghĩa mảng động dùng để chứa hồ sơ trong bộ nhớ.
#include "MangDongBenhNhan.h"
// Dùng luồng lỗi chuẩn khi không đọc được dữ liệu nguồn.
#include <iostream>

// Điều phối quá trình tải, sắp xếp và lưu lại danh sách bệnh nhân.
bool QuanLyHangDoi::taiVaXuLyBenhNhan() {
    // Tạo mảng động chứa các hồ sơ vừa đọc từ cơ sở dữ liệu nguồn.
    MangDongBenhNhan records;
    // Dừng nếu không thể lấy danh sách lượt khám từ cơ sở dữ liệu.
    if (!DBTruyXuat::docDanhSachBenhNhan(records)) {
        // Ghi thông báo lỗi để người dùng biết vì sao luồng bị dừng.
        std::cerr << "Khong the doc danh sach benh nhan tu database.\n";
        return false;
    }

    // Tạo vùng đệm phục vụ quá trình ghép các đoạn trong Merge Sort.
    MangDongBenhNhan buffer;
    // Sao chép các hồ sơ vào bộ đệm để bộ đệm có đủ chỗ cho toàn bộ danh sách.
    for (int index = 0; index < records.size(); ++index)
        buffer.push_back(records[index]);
    // Không gọi sắp xếp nếu danh sách rỗng; nếu có dữ liệu thì sắp xếp toàn đoạn.
    if (!records.empty())
        ThuatToanSapXep::sapXepTron(records, buffer, 0, records.size() - 1);

    // Ghi danh sách đã sắp xếp vào các bảng hàng đợi và trả kết quả thao tác.
    return DBTruyXuat::ghiDanhSachDaSapXep(records);
}