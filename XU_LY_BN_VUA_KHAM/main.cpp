#include <iostream>
#include "patient.h"
#include "FixedCapacityList.h"

int main() {
    // Tạo danh sách với K = 50
    FixedCapacityList recentList = createList(50);

    // Giả lập 3 bệnh nhân vừa khám xong
    Patient p1 = {"BN01", "Nguyen Van A", 1990, "090111111", "Nam", "HCM", "Cam cum"};
    Patient p2 = {"BN02", "Tran Thi B", 1985, "090222222", "Nu", "HCM", "Dau bung"};
    Patient p3 = {"BN03", "Le Van C", 1995, "090333333", "Nam", "HN", "Sot cao"};

    insertExaminedPatient(recentList, p1);
    insertExaminedPatient(recentList, p2);
    insertExaminedPatient(recentList, p3);

    // In danh sách hiện tại
    std::vector<Patient> all = getAll(recentList);
    std::cout << "Danh sach benh nhan vua kham:\n";
    for (const auto& p : all) {
        std::cout << "- " << p.id << " | " << p.name << "\n";
    }

    // Thử xem nhanh 1 người
    Patient found;
    if (getById(recentList, "BN02", found)) {
        std::cout << "\nTim thay: " << found.name << " - " << found.lastDiagnosis << "\n";
    }

    // Dọn dẹp bộ nhớ trước khi kết thúc
    deleteList(recentList);

    return 0;
}