#include "WebService.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <utility>

using hospital_web::Json;
using hospital_web::WebService;

namespace {
std::string readLine(const std::string& label) {
    std::cout << label;
    std::string value;
    if (!std::getline(std::cin, value)) throw std::runtime_error("Da dong luong nhap.");
    return value;
}

int readInt(const std::string& label, int minimum, int maximum) {
    for (;;) {
        const auto text = readLine(label);
        try {
            std::size_t used = 0;
            const int value = std::stoi(text, &used);
            if (used == text.size() && value >= minimum && value <= maximum) return value;
        } catch (const std::exception&) {
        }
        std::cout << "Vui long nhap so tu " << minimum << " den " << maximum << ".\n";
    }
}

bool yes(const std::string& label) {
    for (;;) {
        const auto value = readLine(label + " (c/k): ");
        if (value == "c" || value == "C" || value == "co" || value == "Co") return true;
        if (value == "k" || value == "K" || value == "khong" || value == "Khong") return false;
        std::cout << "Nhap c de co hoac k de khong.\n";
    }
}

std::string text(const Json& item, const char* key, const std::string& fallback = "-") {
    if (!item.contains(key) || item[key].is_null()) return fallback;
    if (item[key].is_string()) return item[key].get<std::string>().empty() ? fallback : item[key].get<std::string>();
    return item[key].dump();
}

void heading(const std::string& title) {
    std::cout << "\n" << std::string(78, '=') << "\n  " << title
              << "\n" << std::string(78, '=') << "\n";
}

void printPatients(const Json& records) {
    if (records.empty()) {
        std::cout << "Khong co ho so benh nhan.\n";
        return;
    }
    std::cout << std::left << std::setw(7) << "ID" << std::setw(28) << "Ho ten"
              << std::setw(14) << "Ngay sinh" << std::setw(16) << "Dien thoai"
              << std::setw(9) << "Tuoi" << "BMI\n";
    for (const auto& p : records)
        std::cout << std::left << std::setw(7) << text(p, "id") << std::setw(28) << text(p, "name")
                  << std::setw(14) << text(p, "birth_date") << std::setw(16) << text(p, "phone")
                  << std::setw(9) << text(p, "age") << text(p, "bmi") << '\n';
    std::cout << "Tong: " << records.size() << " ho so.\n";
}

void printCheckins(const Json& records) {
    if (records.empty()) {
        std::cout << "Chua co phieu check-in.\n";
        return;
    }
    std::cout << std::left << std::setw(10) << "Phieu" << std::setw(9) << "BN"
              << std::setw(26) << "Ho ten" << std::setw(25) << "Khoa"
              << std::setw(10) << "Uu tien" << "Thoi gian\n";
    for (const auto& r : records)
        std::cout << std::left << std::setw(10) << text(r, "checkin_id") << std::setw(9) << text(r, "patient_id")
                  << std::setw(26) << text(r, "patient_name") << std::setw(25) << text(r, "department")
                  << std::setw(10) << text(r, "priority") << text(r, "checkin_time") << '\n';
    std::cout << "Tong: " << records.size() << " phieu.\n";
}

void printQueue(const Json& records) {
    if (records.empty()) {
        std::cout << "Hang doi hien dang trong.\n";
        return;
    }
    std::cout << std::left << std::setw(10) << "Phieu" << std::setw(9) << "BN"
              << std::setw(26) << "Ho ten" << std::setw(25) << "Khoa"
              << std::setw(12) << "Uu tien" << "Check-in\n";
    for (const auto& r : records)
        std::cout << std::left << std::setw(10) << text(r, "checkin_id") << std::setw(9) << text(r, "patient_id")
                  << std::setw(26) << text(r, "patient_name") << std::setw(25) << text(r, "department")
                  << std::setw(12) << text(r, "current_priority") << text(r, "checkin_time") << '\n';
    std::cout << "Tong: " << records.size() << " benh nhan dang cho.\n";
}

void printDoctors(const Json& records, const std::string& department = "") {
    std::size_t count = 0;
    std::cout << std::left << std::setw(12) << "Ma BS" << std::setw(27) << "Ho ten"
              << std::setw(25) << "Khoa" << std::setw(16) << "Trang thai" << "Ca truc\n";
    for (const auto& d : records) {
        if (!department.empty() && text(d, "department") != department) continue;
        std::string status = text(d, "status");
        if (status == "on_duty") status = "Trong ca";
        else if (status == "off_duty") status = "Ngoai ca/nghi";
        else if (status == "busy") status = "Ban dot xuat";
        else if (status == "examining") status = "Dang kham";
        const auto mode = text(d, "duty_mode") == "on_duty" ? "Truc thu cong" :
            text(d, "duty_mode") == "off_duty" ? "Nghi thu cong" : "Theo lich";
        std::cout << std::left << std::setw(12) << text(d, "id") << std::setw(27) << text(d, "name")
                  << std::setw(25) << text(d, "department") << std::setw(16) << status << mode << '\n';
        ++count;
    }
    std::cout << "Tong: " << count << " bac si.\n";
}

void printAssignments(const Json& records) {
    if (records.empty()) {
        std::cout << "Chua co lich phan bac si.\n";
        return;
    }
    std::cout << std::left << std::setw(10) << "Phieu" << std::setw(9) << "BN"
              << std::setw(24) << "Khoa kham" << std::setw(13) << "Ma BS"
              << std::setw(25) << "Bac si" << "Bat dau\n";
    for (const auto& r : records)
        std::cout << std::left << std::setw(10) << text(r, "checkin_id") << std::setw(9) << text(r, "patient_id")
                  << std::setw(24) << text(r, "department") << std::setw(13) << text(r, "doctor_id")
                  << std::setw(25) << text(r, "doctor_name") << text(r, "start_time") << '\n';
    std::cout << "Tong: " << records.size() << " lich.\n";
}

void printExams(const Json& records) {
    if (records.empty()) {
        std::cout << "Khong co luot kham trong danh sach nay.\n";
        return;
    }
    std::cout << std::left << std::setw(10) << "Phieu" << std::setw(9) << "BN"
              << std::setw(24) << "Khoa" << std::setw(13) << "Ma BS"
              << std::setw(25) << "Bac si" << std::setw(22) << "Bat dau" << "Chan doan\n";
    for (const auto& e : records)
        std::cout << std::left << std::setw(10) << text(e, "checkin_id") << std::setw(9) << text(e, "patient_id")
                  << std::setw(24) << text(e, "department") << std::setw(13) << text(e, "doctor_id")
                  << std::setw(25) << text(e, "doctor_name") << std::setw(22) << text(e, "start_time")
                  << text(e, "diagnosis") << '\n';
    std::cout << "Tong: " << records.size() << " luot kham.\n";
}

int chooseDepartment(const Json& departments, bool showStatus) {
    for (std::size_t i = 0; i < departments.size(); ++i) {
        std::cout << (i + 1) << ". " << text(departments[i], "name");
        if (showStatus)
            std::cout << (departments[i].value("accepting_checkins", false) ? " [Dang tiep nhan]" : " [Tam ngung]")
                      << " - " << text(departments[i], "message");
        std::cout << '\n';
    }
    std::cout << "0. Huy\n";
    return readInt("Chon khoa: ", 0, static_cast<int>(departments.size()));
}

void patientsMenu(WebService& service) {
    for (;;) {
        heading("QUAN LY HO SO BENH NHAN");
        std::cout << "1. Danh sach / tim kiem\n2. Them ho so\n3. Sua ho so\n4. Xoa ho so\n0. Quay lai\n";
        const int choice = readInt("Chon: ", 0, 4);
        if (choice == 0) return;
        try {
            if (choice == 1) {
                const auto query = readLine("Tu khoa (bo trong de xem tat ca): ");
                printPatients(service.listPatients(query));
            } else if (choice == 2 || choice == 3) {
                int id = 0;
                Json data = Json::object();
                if (choice == 3) {
                    id = readInt("ID benh nhan: ", 1, std::numeric_limits<int>::max());
                    data = Json::object();
                }
                const auto ask = [&](const char* key, const char* label, bool required = false) {
                    const auto value = readLine(std::string(label) + (required ? " *: " : " (Enter de bo qua): "));
                    if (required || !value.empty()) data[key] = value;
                };
                ask("name", "Ho va ten", choice == 2);
                ask("birth_date", "Ngay sinh (DD/MM/YYYY hoac YYYY-MM-DD)", choice == 2);
                ask("phone", "Dien thoai");
                ask("gender", "Gioi tinh");
                ask("hometown", "Que quan");
                ask("address", "Dia chi");
                for (const auto& field : {std::pair<const char*, const char*>{"height", "Chieu cao cm"},
                                          {"weight", "Can nang kg"}}) {
                    const auto value = readLine(std::string(field.second) + " (Enter de bo qua): ");
                    if (!value.empty()) {
                        try { data[field.first] = std::stod(value); }
                        catch (const std::exception&) { throw std::invalid_argument("Chieu cao/can nang phai la so."); }
                    }
                }
                std::cout << "Da luu ho so:\n" << service.savePatient(data, id).dump(2) << '\n';
            } else {
                const int id = readInt("ID benh nhan can xoa: ", 1, std::numeric_limits<int>::max());
                if (yes("Xac nhan xoa ho so #" + std::to_string(id) + "?"))
                    std::cout << service.deletePatient(id).dump(2) << '\n';
                else std::cout << "Da huy.\n";
            }
        } catch (const std::exception& error) {
            std::cout << "Loi: " << error.what() << '\n';
        }
    }
}

void checkinMenu(WebService& service) {
    for (;;) {
        heading("TIEP NHAN BENH NHAN");
        std::cout << "1. Xem cac phieu check-in\n2. Tao check-in\n0. Quay lai\n";
        const int choice = readInt("Chon: ", 0, 2);
        if (choice == 0) return;
        try {
            if (choice == 1) {
                printCheckins(service.listCheckIns());
                continue;
            }
            const auto departments = service.departments();
            std::cout << "Khoa Cấp cứu tiếp nhận 24/7. Khoa ngoài giờ chỉ chuyển cấp cứu với ưu tiên 1-2.\n";
            const int selected = chooseDepartment(departments, true);
            if (selected == 0) continue;
            Json data = {
                {"patient_id", readInt("ID benh nhan: ", 1, std::numeric_limits<int>::max())},
                {"department", departments[selected - 1]["name"]},
                {"priority", readInt("Muc uu tien (1 cap cuu - 5 thap): ", 1, 5)}
            };
            if (yes("Cho phep chuyen sang Khoa Cap cuu neu khoa da dong (chi uu tien 1-2)?"))
                data["transfer_to_emergency"] = true;
            std::cout << "Check-in thanh cong:\n" << service.checkIn(data).dump(2) << '\n';
        } catch (const std::exception& error) {
            std::cout << "Loi: " << error.what() << '\n';
        }
    }
}

void queueMenu(WebService& service) {
    for (;;) {
        heading("HANG DOI VA MUC UU TIEN");
        std::cout << "1. Xem hang doi\n2. Dong bo hang doi / ca den gio\n3. Doi muc uu tien\n0. Quay lai\n";
        const int choice = readInt("Chon: ", 0, 3);
        if (choice == 0) return;
        try {
            if (choice == 1) printQueue(service.queue());
            else if (choice == 2) {
                const auto result = service.syncExams();
                std::cout << "Da dong bo. ";
                printQueue(result["queue"]);
            } else {
                const int id = readInt("Ma phieu check-in: ", 1, std::numeric_limits<int>::max());
                const int priority = readInt("Muc uu tien moi (1-5): ", 1, 5);
                std::cout << "Da cap nhat:\n"
                          << service.changePriority(id, Json{{"priority", priority}}).dump(2) << '\n';
            }
        } catch (const std::exception& error) {
            std::cout << "Loi: " << error.what() << '\n';
        }
    }
}

void doctorsMenu(WebService& service) {
    for (;;) {
        heading("QUAN LY BAC SI");
        std::cout << "1. Xem bac si theo khoa\n2. Xem ca truc 7 ngay theo ma bac si\n"
                     "3. Cap nhat ca truc / trang thai ban\n0. Quay lai\n";
        const int choice = readInt("Chon: ", 0, 3);
        if (choice == 0) return;
        try {
            const auto doctors = service.listDoctors();
            if (choice == 1) {
                const auto departments = service.departments();
                std::cout << "1. Loc theo khoa\n2. Tat ca cac khoa\n0. Quay lai\n";
                const int scope = readInt("Chon: ", 0, 2);
                if (scope == 0) continue;
                if (scope == 2) printDoctors(doctors);
                else {
                    const int selected = chooseDepartment(departments, false);
                    if (selected != 0) printDoctors(doctors, text(departments[selected - 1], "name"));
                }
            } else {
                const auto id = readLine("Ma bac si: ");
                const auto it = std::find_if(doctors.begin(), doctors.end(),
                    [&](const Json& doctor) { return text(doctor, "id") == id; });
                if (it == doctors.end()) {
                    std::cout << "Khong tim thay bac si.\n";
                    continue;
                }
                if (choice == 2) {
                    std::cout << text(*it, "name") << " - " << text(*it, "department")
                              << " (lich tu " << text(*it, "shift_period_start") << ")\n";
                    for (const auto& shift : (*it)["shifts"])
                        std::cout << (shift.value("is_current", false) ? "* " : "  ")
                                  << text(shift, "date") << "  " << text(shift, "start_time")
                                  << " -> " << text(shift, "end_time")
                                  << (shift.value("is_current", false) ? " [DANG TRUC]" : "") << '\n';
                } else {
                    Json data = Json::object();
                    std::cout << "Ca truc: 1. Theo lich  2. Bat truc thu cong  3. Cho nghi\n";
                    const int mode = readInt("Chon trang thai: ", 1, 3);
                    data["duty_mode"] = mode == 1 ? "auto" : mode == 2 ? "on_duty" : "off_duty";
                    const auto busyText = readLine("So phut ban dot xuat (Enter giu nguyen, 0 de xoa): ");
                    if (!busyText.empty()) {
                        int busy = 0;
                        try {
                            std::size_t used = 0;
                            busy = std::stoi(busyText, &used);
                            if (used != busyText.size() || busy < 0 || busy > 1440)
                                throw std::invalid_argument("So phut ban phai tu 0 den 1440.");
                        } catch (const std::exception&) {
                            throw std::invalid_argument("So phut ban phai la so tu 0 den 1440.");
                        }
                        data["busy_minutes"] = busy;
                        if (busy > 0) data["busy_reason"] = readLine("Ly do: ");
                    }
                    const auto overtimeText = readLine("So phut tang ca (Enter giu nguyen, 0 de huy): ");
                    if (!overtimeText.empty()) {
                        try {
                            std::size_t used = 0;
                            const int overtime = std::stoi(overtimeText, &used);
                            if (used != overtimeText.size() || overtime < 0 || overtime > 720)
                                throw std::invalid_argument("So phut tang ca phai tu 0 den 720.");
                            data["overtime_minutes"] = overtime;
                        } catch (const std::exception&) {
                            throw std::invalid_argument("So phut tang ca phai la so tu 0 den 720.");
                        }
                    }
                    const auto result = service.updateDoctor(id, data);
                    std::cout << "Da cap nhat trang thai bac si:\n" << result.dump(2) << '\n';
                }
            }
        } catch (const std::exception& error) {
            std::cout << "Loi: " << error.what() << '\n';
        }
    }
}

void scheduleMenu(WebService& service) {
    for (;;) {
        heading("LICH PHAN BAC SI");
        std::cout << "1. Xem lich da phan\n2. Phan benh nhan theo mot khoa\n"
                     "3. Phan benh nhan cho mot bac si\n4. Phan tat ca cac khoa\n0. Quay lai\n";
        const int choice = readInt("Chon: ", 0, 4);
        if (choice == 0) return;
        try {
            if (choice == 1) {
                printAssignments(service.listAssignments());
                continue;
            }
            Json data = Json::object();
            if (choice == 2) {
                const auto departments = service.departments();
                const int selected = chooseDepartment(departments, false);
                if (selected == 0) continue;
                data["department"] = departments[selected - 1]["name"];
            } else if (choice == 3) {
                printDoctors(service.listDoctors());
                data["doctor_id"] = readLine("Ma bac si muon nhan benh nhan: ");
            }
            const auto result = service.schedule(data);
            std::cout << "Da phan " << result.value("assigned_count", 0)
                      << " ca; con " << result.value("waiting_count", 0) << " benh nhan cho.\n";
            printAssignments(result["assignments"]);
        } catch (const std::exception& error) {
            std::cout << "Loi: " << error.what() << '\n';
        }
    }
}

void examsMenu(WebService& service) {
    for (;;) {
        heading("DANG KHAM VA LICH SU KHAM");
        std::cout << "1. Xem ca dang kham\n2. Dong bo ca den gio\n3. Cap nhat chan doan / don thuoc\n"
                     "4. Ket thuc luot kham\n5. Xem lich su kham\n0. Quay lai\n";
        const int choice = readInt("Chon: ", 0, 5);
        if (choice == 0) return;
        try {
            if (choice == 1) printExams(service.listExams(true));
            else if (choice == 2) {
                const auto result = service.syncExams();
                std::cout << "Da dong bo danh sach ca kham.\n";
                printExams(result["active_exams"]);
            } else if (choice == 5) printExams(service.listExams(false));
            else if (choice == 3) {
                const int id = readInt("Ma phieu dang kham: ", 1, std::numeric_limits<int>::max());
                Json diagnosis = {
                    {"diagnosis", readLine("Chan doan: ")},
                    {"prescription", readLine("Don thuoc (co the bo trong): ")},
                    {"reminder", readLine("Loi dan / hen tai kham (co the bo trong): ")}
                };
                std::cout << "Da luu:\n" << service.diagnosis(id, diagnosis).dump(2) << '\n';
            } else {
                const int id = readInt("Ma phieu dang kham can ket thuc: ", 1, std::numeric_limits<int>::max());
                if (yes("Xac nhan ket thuc luot kham #" + std::to_string(id) + "?"))
                    std::cout << service.finishExam(id).dump(2) << '\n';
                else std::cout << "Da huy.\n";
            }
        } catch (const std::exception& error) {
            std::cout << "Loi: " << error.what() << '\n';
        }
    }
}

void overview(WebService& service) {
    const auto patients = service.listPatients();
    const auto checkins = service.listCheckIns();
    const auto queue = service.queue();
    const auto doctors = service.listDoctors();
    const auto assignments = service.listAssignments();
    const auto active = service.listExams(true);
    const auto history = service.listExams(false);
    heading("TONG QUAN HE THONG");
    std::cout << "Ho so benh nhan       : " << patients.size() << '\n'
              << "Phieu tiep nhan       : " << checkins.size() << '\n'
              << "Benh nhan trong hang doi: " << queue.size() << '\n'
              << "Bac si                : " << doctors.size() << '\n'
              << "Lich da phan           : " << assignments.size() << '\n'
              << "Ca dang kham           : " << active.size() << '\n'
              << "Luot kham (ca dang kham + lich su): " << history.size() << '\n';
    std::cout << "\nLuong nghiep vu: Ho so -> Tiep nhan -> Hang doi -> Phan bac si -> "
                 "Dang kham -> Chan doan -> Ket thuc -> Lich su.\n";
}

void menu() {
    std::cout << "\n  MEDIFLOW - QUAN LY KHAM BENH (TERMINAL)\n"
                 "  1. Tong quan\n"
                 "  2. Ho so benh nhan\n"
                 "  3. Tiep nhan / check-in\n"
                 "  4. Hang doi / uu tien\n"
                 "  5. Bac si / ca truc\n"
                 "  6. Lich phan bac si\n"
                 "  7. Dang kham / lich su\n"
                 "  0. Thoat\n";
}
}

int main(int argc, char** argv) {
    try {
#ifdef _WIN32
        if (std::system("chcp 65001 > nul < nul") != 0)
            throw std::runtime_error("Khong the chuyen terminal sang ma UTF-8.");
#endif
        if (argc > 2) throw std::invalid_argument("Cach dung: hospital_terminal.exe [repo-root]");
        if (argc == 2) std::filesystem::current_path(std::filesystem::absolute(argv[1]));
        if (!std::filesystem::exists("SAP_XEP_BAC_SI/src"))
            throw std::runtime_error("Hay chay tu root repo hoac truyen repo-root.");
        WebService service;
        std::cout << "Da ket noi du lieu MediFlow. Du lieu dung chung voi ung dung web.\n";
        for (;;) {
            menu();
            const int choice = readInt("Chon chuc nang: ", 0, 7);
            try {
                if (choice == 0) {
                    std::cout << "Tam biet.\n";
                    return 0;
                }
                if (choice == 1) overview(service);
                else if (choice == 2) patientsMenu(service);
                else if (choice == 3) checkinMenu(service);
                else if (choice == 4) queueMenu(service);
                else if (choice == 5) doctorsMenu(service);
                else if (choice == 6) scheduleMenu(service);
                else if (choice == 7) examsMenu(service);
            } catch (const std::exception& error) {
                std::cout << "Loi: " << error.what() << '\n';
            }
        }
    } catch (const std::exception& error) {
        if (std::string(error.what()) != "Da dong luong nhap.") std::cerr << "Loi: " << error.what() << '\n';
        return std::string(error.what()) == "Da dong luong nhap." ? 0 : 1;
    }
}
