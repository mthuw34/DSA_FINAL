#include "ClinicQueueManager.h"
#include <crow_all.h>

int main() {
    crow::SimpleApp app;
    ClinicQueueManager queue; // Khởi tạo Cấu trúc dữ liệu

    CROW_STATIC_FILE(app, "/", "TRUY_XUAT_BENH_NHAN/static/index.html");
    CROW_STATIC_FILE(app, "/static/style.css", "TRUY_XUAT_BENH_NHAN/static/style.css");
    CROW_STATIC_FILE(app, "/static/app.js", "TRUY_XUAT_BENH_NHAN/static/app.js");

    // API Endpoint: /api/call-next
    CROW_ROUTE(app, "/api/call-next")
    ([&queue](){
        return queue.callNextPatientWeb(); // Gọi hàm xử lý và trả về JSON
    });

    // API xem hàng chờ theo từng khoa, không lấy bệnh nhân ra khỏi hàng
    CROW_ROUTE(app, "/api/queues")
    ([&queue](){
        return queue.getQueuesByDepartmentWeb();
    });

    app.port(8080).multithreaded().run();
    return 0;
}