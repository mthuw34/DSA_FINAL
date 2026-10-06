// Khai báo lớp quản lý hàng đợi và các kiểu dữ liệu của chức năng truy xuất.
#include "TRUY_XUAT/TruyXuat.h"
// Khai báo các hàm xóa một hoặc toàn bộ bệnh nhân.
#include "XOA_BENH_NHAN/XoaBenhNhan.h"
// Dùng các luồng nhập/xuất để giao tiếp với người dùng.
#include <iostream>
// Dùng giới hạn streamsize khi xóa phần nhập sai khỏi bộ đệm.
#include <limits>

// Cho phép sử dụng tên cout, cin, cerr mà không cần tiền tố std::.
using namespace std;

// Điểm bắt đầu của chương trình truy xuất bệnh nhân.
int main() {
    // Tạo đối tượng điều phối thao tác tải và sắp xếp hàng đợi.
    QuanLyHangDoi manager;

    // Lặp lại menu cho đến khi người dùng chọn thoát.
    while (true) {
        // In tiêu đề và các chức năng có thể chọn.
        cout << "\n========== TRUY XUAT BENH NHAN ==========\n";
        cout << "1. Truy xuat va sap xep benh nhan\n";
        cout << "2. Xoa mot benh nhan\n";
        cout << "3. Xoa toan bo benh nhan\n";
        cout << "0. Thoat\n";
        // Yêu cầu người dùng nhập lựa chọn menu.
        cout << "Lua chon: ";

        // Gán giá trị mặc định để nhận biết trường hợp nhập không hợp lệ.
        int choice = -1;
        // Đọc lựa chọn; nếu không nhập được số nguyên thì xử lý lỗi nhập.
        if (!(cin >> choice)) {
            // Xóa trạng thái lỗi của cin để có thể tiếp tục nhận dữ liệu.
            cin.clear();
            // Bỏ phần còn lại của dòng nhập sai khỏi bộ đệm đầu vào.
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            // Thông báo lỗi rồi quay lại đầu vòng lặp để hiện menu.
            cout << "Lua chon khong hop le.\n";
            continue;
        }

        // Lựa chọn 0 kết thúc vòng lặp menu.
        if (choice == 0) {
            break;
        }

        // Lựa chọn 1 đọc dữ liệu và sắp xếp lại danh sách bệnh nhân.
        if (choice == 1) {
            // Gọi lớp quản lý và thông báo theo kết quả trả về.
            if (manager.taiVaXuLyBenhNhan()) {
                cout << "Da truy xuat va sap xep du lieu thanh cong!\n";
            } else {
                cerr << "Khong the truy xuat va sap xep du lieu!\n";
            }
        // Lựa chọn 2 yêu cầu mã bệnh nhân rồi gọi chức năng xóa một người.
        } else if (choice == 2) {
            // Hiển thị lời nhắc nhập mã bệnh nhân cần xóa.
            cout << "Nhap ID benh nhan can xoa: ";
            // Khởi tạo biến nhận mã bệnh nhân.
            int patientId = 0;
            // Nếu dữ liệu nhập không phải số nguyên, khôi phục cin và báo lỗi.
            if (!(cin >> patientId)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "ID benh nhan khong hop le.\n";
                continue;
            }
            // Chuyển yêu cầu xóa xuống lớp xử lý nghiệp vụ/cơ sở dữ liệu.
            XoaBenhNhan::xoaBenhNhan(patientId);
        // Lựa chọn 3 chỉ xóa toàn bộ sau khi người dùng xác nhận bằng Y/y.
        } else if (choice == 3) {
            // Cảnh báo thao tác và yêu cầu xác nhận trước khi xóa toàn bộ.
            cout << "Ban co chac chan muon xoa toan bo benh nhan? "
                    "Nhap Y de xac nhan: ";
            // Khởi tạo ký tự xác nhận, mặc định chưa có lựa chọn.
            char confirmation = '\0';
            // Nếu không đọc được ký tự, dọn trạng thái nhập và thử lại menu.
            if (!(cin >> confirmation)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Lua chon khong hop le.\n";
                continue;
            }
            // Chỉ gọi thao tác xóa khi người dùng xác nhận chữ Y hoa hoặc thường.
            if (confirmation == 'Y' || confirmation == 'y') {
                XoaBenhNhan::xoaTatCaBenhNhan();
            } else {
                // Mọi ký tự xác nhận khác đều hủy thao tác xóa.
                cout << "Da huy thao tac xoa.\n";
            }
        // Các số không thuộc danh sách lựa chọn được báo là không hợp lệ.
        } else {
            cout << "Lua chon khong hop le.\n";
        }
    }

    // Trả mã 0 để báo chương trình kết thúc bình thường.
    return 0;
}