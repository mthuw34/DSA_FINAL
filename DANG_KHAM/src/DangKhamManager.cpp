#include "DangKhamManager.h"

#include <iostream>
#include <limits>

#include "NhapChanDoan.h"

using namespace std;

// Khởi động module DANG_KHAM và đồng bộ dữ liệu bác sĩ đã được xếp.
bool DangKhamManager::khoiDong(
    const string& sourcePath,
    const string& destinationPath
)
{
    if (!database.mo(
            sourcePath,
            destinationPath
        ))
    {
        return false;
    }

    if (!database.taoCauTruc())
        return false;

    if (!database.dongBoTuXepBacSi())
        return false;

    return true;
}

// Hiển thị các ca được tầng C++ lọc và sắp xếp trong bộ nhớ.
void DangKhamManager::hienThiDangKham() {
    vector<ExamSession> records;
    if (!database.docDanhSach(records)) {
        cerr << "Loi doc danh sach dang kham.\n"; return;
    }
    const auto active = ExamCore::activeSessions(records);
    cout << "\n========== BENH NHAN DANG KHAM ==========\n";
    int count = 0;
    for (const auto& session : active) {
        cout << ++count << ". Check-in: " << session.checkinId
             << " | BN: " << session.patientId << " | " << session.department
             << " | BS: " << session.doctorId.value_or("") << " - " << session.doctorName.value_or("")
             << " | Bat dau: " << session.startTime.value_or("")
             << "\n   Chan doan: " << session.diagnosis.value_or("Chua nhap")
             << "\n   Don thuoc: " << session.prescription.value_or("Chua nhap")
             << "\n   Loi nhac bac si: " << session.reminder.value_or("Chua nhap") << '\n';
    }
    if (active.empty()) cout << "Khong co benh nhan nao dang kham.\n";
}

// Tìm check-in trong danh sách đã nạp và kiểm tra trạng thái bằng C++.
bool DangKhamManager::layPhienDangKham(int checkinId, int& patientId) {
    vector<ExamSession> records;
    if (!database.docDanhSach(records)) return false;
    const auto* session = ExamCore::findActive(records, checkinId);
    if (!session) return false;
    patientId = session->patientId;
    return true;
}

// Nhập và lưu chẩn đoán, đơn thuốc và lời nhắc của bác sĩ.
void DangKhamManager::nhapChanDoan()
{
    cout << "Nhap check-in ID: ";

    int checkinId = 0;

    if (!(cin >> checkinId) ||
        checkinId <= 0)
    {
        cin.clear();

        cin.ignore(
            numeric_limits<streamsize>::max(),
            '\n'
        );

        cout
            << "Check-in ID khong hop le.\n";

        return;
    }

    int patientId = 0;

    if (!layPhienDangKham(
            checkinId,
            patientId
        ))
    {
        cout
            << "Benh nhan khong o trang thai dang kham.\n";

        return;
    }

    string chanDoan;
    string donThuoc;
    string loiNhacBacSi;

    NhapChanDoan(
        to_string(patientId),
        chanDoan,
        donThuoc,
        loiNhacBacSi
    );

    if (!cin || chanDoan.find_first_not_of(" \t\r\n") == string::npos)
    {
        cout
            << "Chan doan khong duoc de trong.\n";

        return;
    }

    if (database.luuChanDoan(checkinId, chanDoan, donThuoc, loiNhacBacSi))
        cout << "Da luu chan doan, don thuoc va loi nhac bac si.\n";
    else cerr << "Khong cap nhat duoc thong tin kham.\n";
}

// Kết thúc phiên khám và ghi thời gian kết thúc.
void DangKhamManager::ketThucKham()
{
    cout
        << "Nhap check-in ID ket thuc kham: ";

    int checkinId = 0;

    if (!(cin >> checkinId) ||
        checkinId <= 0)
    {
        cin.clear();

        cin.ignore(
            numeric_limits<streamsize>::max(),
            '\n'
        );

        cout
            << "Check-in ID khong hop le.\n";

        return;
    }

    int patientId = 0;
    if (!layPhienDangKham(checkinId, patientId)) {
        cout << "Khong tim thay phien dang kham phu hop.\n";
        return;
    }
    if (database.ketThucPhien(checkinId)) cout << "Da ket thuc phien kham.\n";
    else cout << "Khong the ket thuc phien kham.\n";
}

// Hiển thị và xử lý menu chính của module DANG_KHAM.
void DangKhamManager::chayMenu()
{
    while (true)
    {
        cout
            << "\n========== DANG KHAM ==========\n"
            << "1. Xem benh nhan dang kham\n"
            << "2. Nhap chan doan, don thuoc va loi nhac bac si\n"
            << "3. Ket thuc kham\n"
            << "0. Thoat\n"
            << "Lua chon: ";

        int choice = -1;

        if (!(cin >> choice))
        {
            if (cin.eof() ||
                cin.bad())
            {
                break;
            }

            cin.clear();

            cin.ignore(
                numeric_limits<streamsize>::max(),
                '\n'
            );

            cout
                << "Lua chon khong hop le.\n";

            continue;
        }

        if (choice == 0)
            break;

        // Nhận ca vừa bắt đầu khi chương trình đang mở.
        if (choice >= 1 && choice <= 3 && !database.dongBoTuXepBacSi())
            continue;

        if (choice == 1)
            hienThiDangKham();
        else if (choice == 2)
            nhapChanDoan();
        else if (choice == 3)
            ketThucKham();
        else
            cout
                << "Lua chon khong hop le.\n";
    }
}
