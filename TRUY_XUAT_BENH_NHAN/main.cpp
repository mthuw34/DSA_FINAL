<<<<<<< HEAD
#include "QuanLyHangDoi.h"
#include <iostream>

using namespace std;

int main() {
    // Khởi tạo đối tượng quản lý, quá trình lấy dữ liệu và sắp xếp sẽ tự động chạy
    QuanLyHangDoi manager;

    if (!manager.xuLyThanhCong()) {
        cerr << "Khong the tao TRUY_XUAT_BENH_NHAN/truyXuat.db.\n";
        return 1;
    }

    cout << "Da tao TRUY_XUAT_BENH_NHAN/truyXuat.db thanh cong!\n";
=======
#include "ClinicQueueManager.h"
#include <crow_all.h>

int main() {
    crow::SimpleApp app;
    ClinicQueueManager queue; // Khởi tạo Cấu trúc dữ liệu

    // API Endpoint: /api/call-next
    CROW_ROUTE(app, "/api/call-next")
    ([&queue](){
        return queue.callNextPatientWeb(); // Gọi hàm xử lý và trả về JSON
    });

    app.port(8080).multithreaded().run();
>>>>>>> fbbd61a3fe61e2c993e3a3a7b581933e1de1aea1
    return 0;
}