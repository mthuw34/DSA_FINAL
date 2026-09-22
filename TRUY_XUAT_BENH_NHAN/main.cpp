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

    // API xem hàng chờ theo từng khoa, không lấy bệnh nhân ra khỏi hàng
    CROW_ROUTE(app, "/api/queues")
    ([&queue](){
        return queue.getQueuesByDepartmentWeb();
    });

    app.port(8080).multithreaded().run();
    return 0;
}