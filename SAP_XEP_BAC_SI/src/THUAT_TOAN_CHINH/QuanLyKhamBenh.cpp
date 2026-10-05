#include "QuanLyKhamBenh.h"
#include "ThoiGian.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <chrono>
#include "ExamCore.h"
using namespace std;

namespace {
    const string CAC_KHOA[] = {
        "Khoa Cap cuu",
        "Khoa Noi",
        "Khoa Ngoai",
        "Khoa Tim mach",
        "Khoa Nhi",
        "Khoa San",
        "Khoa Tai Mui Hong",
        "Khoa Mat",
        "Khoa Da lieu",
        "Khoa Than kinh"
    };

    string tenKhoaDep(const string& khoa) {
        return khoa;
    }

    struct UngVienBacSi {
        int index = -1;
        time_t batDau = 0;
    };

    struct SoSanhUngVien {
        bool operator()(
            const UngVienBacSi& a,
            const UngVienBacSi& b
        ) const {
            if (a.batDau != b.batDau) {
                return a.batDau < b.batDau;
            }
            return a.index < b.index;
        }
    };
}

int QuanLyKhamBenh::TimChiSoKhoa(const string& khoa) const {
    for (int i = 0; i < SO_KHOA; ++i) {
        if (CAC_KHOA[i] == khoa) return i;
    }
    return -1;
}

bool QuanLyKhamBenh::KhoiDong(
    const string& duongDanDatabase,
    const string& duongDanCSV
) {
    if (!QuanLyBacSiManager.DocCSV(duongDanCSV)) {
        return false;
    }

    if (!Database.MoDatabase(duongDanDatabase)) {
        return false;
    }

    time_t hienTai = ThoiGian::HienTai();
    QuanLyBacSiManager.KhoiTaoLich(hienTai);

    // Database chỉ đc đọc 1 lần -> nạp vào RAM
    NapHangDoiTuDatabase();

    cout << "Thoi gian he thong: "
              << ThoiGian::DinhDang(hienTai) << "\n";

    return true;
}

void QuanLyKhamBenh::NapHangDoiTuDatabase() {
    vector<BenhNhanKham> tatCaBenhNhan;

    if (!Database.LayTatCaBenhNhan(tatCaBenhNhan)) {
        cerr << "Khong nap duoc toan bo hang doi benh nhan.\n";
        return;
    }

    // Ko phụ thuộc vào thứ tự SQLite trả về
    // Sau khi nạp dô RAM, sắp xếp theo khoa rồi đến retrieval_order
    stable_sort(
        tatCaBenhNhan.begin(),
        tatCaBenhNhan.end(),
        [](const BenhNhanKham& a, const BenhNhanKham& b) {
            if (a.khoa != b.khoa) return a.khoa < b.khoa;
            return a.RetrievalOrder < b.RetrievalOrder;
        }
    );

    int soNap = 0;

    for (const BenhNhanKham& bn : tatCaBenhNhan) {
        if (find(CheckinDaPhan.begin(), CheckinDaPhan.end(), bn.CheckinId) != CheckinDaPhan.end()) continue;
        int index = TimChiSoKhoa(bn.khoa);

        if (index < 0) {
            cerr << "Bo qua benh nhan co khoa khong hop le: "
                      << bn.khoa << "\n";
            continue;
        }

        HangDoiTheoKhoa[index].push(bn);
        ++soNap;
    }

    cout << "Da nap " << soNap
              << " benh nhan vao cac hang doi RAM.\n";
}

int QuanLyKhamBenh::RandomThoiGianKham() const {
    static mt19937 gen(
        static_cast<unsigned int>(
            chrono::system_clock::now().time_since_epoch().count()
        )
    );

    uniform_int_distribution<int> dist(10, 30);
    return dist(gen);
}

void QuanLyKhamBenh::InDanhSachBacSiTheoKhoa(
    const string& khoa
) const {
    vector<int> danhSach =
        QuanLyBacSiManager.LayBacSiTheoKhoa(khoa);

    cout << "\n===== " << tenKhoaDep(khoa) << " =====\n";

    if (danhSach.empty()) {
        cout << "Khong co bac si nao trong khoa.\n";
        return;
    }

    for (int index : danhSach) {
        const BacSi& bs = QuanLyBacSiManager.LayBacSi(index);

        cout
            << bs.id << " | "
            << bs.name
            << " | Kinh nghiem: "
            << bs.ExpYears << " nam\n";
    }
}

bool QuanLyKhamBenh::TimBacSiTotNhat(
    const string& khoa,
    time_t hienTai,
    int thoiLuong,
    int& bacSiIndex,
    time_t& batDau
) {
    // MinHeap: phần tử đầu là bsi có thời điểm bắt đầu sớm nhất
    MinHeapTuCaiDat<UngVienBacSi, SoSanhUngVien> minHeap;

    vector<int> danhSach =
        QuanLyBacSiManager.LayBacSiTheoKhoa(khoa);

    for (int index : danhSach) {
        if (WebMode && find(BacSiChoPhep.begin(), BacSiChoPhep.end(),
                QuanLyBacSiManager.LayBacSi(index).id) == BacSiChoPhep.end()) continue;
        time_t thoiDiemNhan = 0;

        if (QuanLyBacSiManager.TinhThoiDiemNhanBenhNhan(
                index,
                khoa,
                hienTai,
                thoiLuong,
                thoiDiemNhan)) {

            minHeap.push({index, thoiDiemNhan});
        }
    }

    if (minHeap.empty()) {
        return false;
    }

    const UngVienBacSi& best = minHeap.top();
    bacSiIndex = best.index;
    batDau = best.batDau;

    return true;
}

bool QuanLyKhamBenh::XuLyMotBenhNhan(
    BenhNhanKham& benhNhan,
    const string& khoaThucTe,
    time_t hienTai
) {
    int thoiLuong = RandomThoiGianKham();

    int bacSiIndex = -1;
    time_t batDau = 0;

    if (!TimBacSiTotNhat(
            khoaThucTe,
            hienTai,
            thoiLuong,
            bacSiIndex,
            batDau)) {

        benhNhan.Status = "CHO_DOI";
        benhNhan.Note = "Chua tim duoc bac si phu hop.";
        return false;
    }

    BacSi& bs = QuanLyBacSiManager.LayBacSi(bacSiIndex);
    time_t ketThuc = batDau + thoiLuong * 60;

    if (!bs.TrucThuCong && khoaThucTe != "Khoa Cap cuu" &&
        !ThoiGian::DuThoiGianKhamKhoaThuong(
            batDau,
            thoiLuong
        )) {

        benhNhan.Status = "CHO_DOI";
        benhNhan.Note =
            "Khong du thoi gian trong ca lam viec hien tai.";
        return false;
    }

    benhNhan.DoctorId = bs.id;
    benhNhan.DoctorName = bs.name;
    benhNhan.KhoaBacSi = bs.khoaChuyenMon;
    benhNhan.StartTime = ThoiGian::DinhDang(batDau);
    benhNhan.ExamDuration = thoiLuong;
    benhNhan.EndTime = ThoiGian::DinhDang(ketThuc);
    benhNhan.Status = "DA_XEP_BAC_SI";

    if (benhNhan.khoa != khoaThucTe) {
        benhNhan.Note =
            "Benh nhan duoc chuyen sang Khoa Cap cuu do khoa ban dau nghi.";
    } else {
        benhNhan.Note = "Xep bac si theo thoi diem ranh som nhat.";
    }

    QuanLyBacSiManager.CapNhatSauKhiKham(
        bacSiIndex,
        ketThuc
    );

    return true;
}

bool QuanLyKhamBenh::XuLyKhoa(
    const string& khoa
) {
    int khoaIndex = TimChiSoKhoa(khoa);
    if (khoaIndex < 0) {
        cout << "Khoa khong hop le.\n";
        return false;
    }

    time_t hienTai = ThoiGian::HienTai();

    cout << "\n========================================\n";
    cout << "XU LY " << tenKhoaDep(khoa) << "\n";
    cout << "Thoi gian hien tai: "
              << ThoiGian::DinhDang(hienTai) << "\n";

    HangDoiTuCaiDat<BenhNhanKham>& hangDoi =
        HangDoiTheoKhoa[khoaIndex];

    if (hangDoi.empty()) {
        cout << "Hang doi dang rong.\n";
        return true;
    }

    cout << "So benh nhan trong hang: "
              << hangDoi.size() << "\n";

    vector<BenhNhanKham> ketQuaKhoa;

    // Cuối tuần khoa thường nghỉ -> chuyển BN sang khoa cấp cứu
    if (!WebMode && khoa != "Khoa Cap cuu" &&
        ThoiGian::LaCuoiTuan(hienTai)) {

        cout
            << "Hom nay la cuoi tuan. "
            << "Benh nhan duoc chuyen sang Khoa Cap cuu.\n";

        while (!hangDoi.empty()) {
            BenhNhanKham bn = hangDoi.front();
            hangDoi.pop();

            if (XuLyMotBenhNhan(
                    bn,
                    "Khoa Cap cuu",
                    hienTai)) {
                ketQuaKhoa.push_back(bn);
                KetQuaTrongLanChay.push_back(bn);
            }
        }
    } else {
        int soDaXuLy = 0;
        int soCho = 0;

        while (!hangDoi.empty()) {
            BenhNhanKham bn = hangDoi.front();
            hangDoi.pop();

            if (XuLyMotBenhNhan(
                    bn,
                    khoa,
                    hienTai)) {

                ++soDaXuLy;
                ketQuaKhoa.push_back(bn);
                KetQuaTrongLanChay.push_back(bn);

                cout
                    << "\nBN checkin_id = "
                    << bn.CheckinId
                    << " -> "
                    << bn.DoctorId
                    << " (" << bn.DoctorName << ")\n"
                    << "Bat dau: " << bn.StartTime << "\n"
                    << "Thoi luong: " << bn.ExamDuration << " phut\n"
                    << "Ket thuc: " << bn.EndTime << "\n";
            } else {
                ++soCho;

                cout
                    << "\nBN checkin_id = "
                    << bn.CheckinId
                    << " chua xep duoc bac si.\n";

                // Bảo toàn BN, tránh lặp vô hạn
                hangDoi.push(bn);
                break;
            }
        }

        cout
            << "\nTong ket: "
            << soDaXuLy << " benh nhan da xep, "
            << soCho << " benh nhan dang cho.\n";
    }

    // Ket qua phan bac si duoc giu trong RAM.
    // Web se ghi truc tiep vao DANG_KHAM sau khi scheduler ket thuc.
    return true;
}

bool QuanLyKhamBenh::XuLyTatCaKhoa() {
    for (int i = 0; i < SO_KHOA; ++i) {
        if (!XuLyKhoa(CAC_KHOA[i])) return false;
    }
    return true;
}

bool QuanLyKhamBenh::KhoiDongWeb(const string& database, const string& csv,
    const vector<BenhNhanKham>& daPhan, const vector<string>& choPhep,
    const vector<string>& dangTruc) {
    if (!QuanLyBacSiManager.DocCSV(csv) || !Database.MoDatabase(database)) return false;
    WebMode = true;
    BacSiChoPhep = choPhep;
    const auto now = ThoiGian::HienTai();
    QuanLyBacSiManager.KhoiTaoLich(now);
    for (int i = 0; i < static_cast<int>(QuanLyBacSiManager.LayDanhSach().size()); ++i) {
        auto& doctor = QuanLyBacSiManager.LayBacSi(i);
        if (find(dangTruc.begin(), dangTruc.end(), doctor.id) != dangTruc.end()) {
            doctor.TrucThuCong = true;
            doctor.ThoiGianRanh = now;
        }
        for (const auto& appointment : daPhan) {
            time_t end;
            if (appointment.DoctorId == doctor.id && ExamCore::parseTime(appointment.EndTime, end))
                doctor.ThoiGianRanh = max(doctor.ThoiGianRanh, end);
        }
    }
    for (const auto& appointment : daPhan) CheckinDaPhan.push_back(appointment.CheckinId);
    NapHangDoiTuDatabase();
    return true;
}

void QuanLyKhamBenh::HienThiKetQua() const {
    cout << "\n===== KET QUA TRONG PHIEN CHAY =====\n";

    if (KetQuaTrongLanChay.empty()) {
        cout << "Chua co ket qua.\n";
        return;
    }

    for (const BenhNhanKham& bn : KetQuaTrongLanChay) {
        cout
            << "Checkin: " << bn.CheckinId
            << " | BN: " << bn.PatientId
            << " | Khoa: " << bn.khoa
            << " | BS: " << bn.DoctorId
            << " - " << bn.DoctorName
            << " | Bat dau: " << bn.StartTime
            << " | Ket thuc: " << bn.EndTime
            << " | " << bn.Status
            << " | " << bn.Note
            << "\n";
    }
}
