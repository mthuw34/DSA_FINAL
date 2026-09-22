#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <stdexcept>

// Cấu trúc lượt khám
struct Appointment {
    std::string appointment_code;
    std::string patient_code;
    int priority_level; // 1 đến 5 (5 là cấp cứu nguy kịch nhất)
    // Các thông tin khác...
};

class ClinicQueueManager {
private:
    // Mảng chứa 5 danh sách liên kết, mỗi danh sách đại diện cho 1 mức ưu tiên.
    // Index 1 đến 5 tương ứng với priority_level 1 đến 5. (Kích thước mảng = 6 để bỏ qua index 0)
    std::vector<std::list<Appointment>> waitlists;

public:
    ClinicQueueManager() {
        waitlists.resize(6); 
    }

    // 1. Thêm bệnh nhân vào hàng chờ (Thêm vào cuối danh sách của mức ưu tiên tương ứng - O(1))
    void addPatient(const Appointment& appt) {
        if (appt.priority_level >= 1 && appt.priority_level <= 5) {
            waitlists[appt.priority_level].push_back(appt);
            std::cout << "Da them benh nhan: " << appt.patient_code << " vao muc uu tien " << appt.priority_level << "\n";
        }
    }

    // 2. MC2: Gọi bệnh nhân tiếp theo (O(1))
    Appointment callNextPatient() {
        // Duyệt từ mức ưu tiên cao nhất (5) xuống thấp nhất (1)
        for (int i = 5; i >= 1; --i) {
            if (!waitlists[i].empty()) {
                // Lấy người đứng đầu hàng đợi của mức ưu tiên này (Đến trước khám trước)
                Appointment next_patient = waitlists[i].front();
                waitlists[i].pop_front(); // Xóa khỏi hàng chờ
                return next_patient;
            }
        }
        throw std::runtime_error("Hien tai khong co benh nhan nao dang cho.");
    }
};

int main() {
    ClinicQueueManager queue;

    // Giả lập bệnh nhân đăng ký
    queue.addPatient({"AP-01", "BN-8892", 2}); // Mức thường, đến sớm
    queue.addPatient({"AP-02", "BN-1122", 5}); // Cấp cứu (sẽ được gọi đầu tiên)
    queue.addPatient({"AP-03", "BN-3344", 2}); // Mức thường, đến sau BN-8892

    // Gọi khám
    try {
        Appointment next = queue.callNextPatient();
        std::cout << "\nGoi kham: " << next.patient_code << " (Uu tien: " << next.priority_level << ")\n";
        
        next = queue.callNextPatient();
        std::cout << "Goi kham: " << next.patient_code << " (Uu tien: " << next.priority_level << ")\n";
    } catch (const std::exception& e) {
        std::cout << e.what() << '\n';
    }

    return 0;
}