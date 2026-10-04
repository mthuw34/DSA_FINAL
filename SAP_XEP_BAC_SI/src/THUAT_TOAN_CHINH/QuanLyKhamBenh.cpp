#include "QuanLyKhamBenh.h"
#include "ThoiGian.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <chrono>

/*
TimChiSoKhoa()             : dòng 58
KhoiDong()                 : dòng 65
NapHangDoiTuDatabase()     : dòng 89
RandomThoiGianKham()       : dòng 127
InDanhSachBacSiTheoKhoa()  : dòng 138
TimBacSiTotNhat()          : dòng 162
XuLyMotBenhNhan()          : dòng 200
XuLyKhoa()                 : dòng 260
XuLyTatCaKhoa()            : dòng 371
HienThiKetQua()            : dòng 377
*/

namespace {
    // Dung cung dinh dang voi khoa_chuyen_mon trong CSV va department trong DB.
    const std::string CAC_KHOA[] = {
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

    std::string tenKhoaDep(const std::string& khoa) {
        return khoa;
    }

    struct UngVienBacSi {
        int index = -1;
        std::time_t batDau = 0;
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

int QuanLyKhamBenh::TimChiSoKhoa(const std::string& khoa) const {
    for (int i = 0; i < SO_KHOA; ++i) {
        if (CAC_KHOA[i] == khoa) return i;
    }
    return -1;
}

bool QuanLyKhamBenh::KhoiDong(
    const std::string& duongDanDatabase,
    const std::string& duongDanCSV
) {
    if (!QuanLyBacSiManager.DocCSV(duongDanCSV)) {
        return false;
    }

    if (!Database.MoDatabase(duongDanDatabase)) {
        return false;
    }

    std::time_t hienTai = ThoiGian::HienTai();
    QuanLyBacSiManager.KhoiTaoLich(hienTai);

    // Database chi duoc doc mot lan: nap toan bo benh nhan vao RAM.
    NapHangDoiTuDatabase();

    std::cout << "Thoi gian he thong: "
              << ThoiGian::DinhDang(hienTai) << "\n";

    return true;
}

void QuanLyKhamBenh::NapHangDoiTuDatabase() {
    std::vector<BenhNhanKham> tatCaBenhNhan;

    if (!Database.LayTatCaBenhNhan(tatCaBenhNhan)) {
        std::cerr << "Khong nap duoc toan bo hang doi benh nhan.\n";
        return;
    }

    // Khong phu thuoc vao thu tu SQLite tra ve.
    // Sau khi nap vao RAM, sap xep theo khoa roi den retrieval_order.
    std::stable_sort(
        tatCaBenhNhan.begin(),
        tatCaBenhNhan.end(),
        [](const BenhNhanKham& a, const BenhNhanKham& b) {
            if (a.khoa != b.khoa) return a.khoa < b.khoa;
            return a.RetrievalOrder < b.RetrievalOrder;
        }
    );

    int soNap = 0;

    for (const BenhNhanKham& bn : tatCaBenhNhan) {
        int index = TimChiSoKhoa(bn.khoa);

        if (index < 0) {
            std::cerr << "Bo qua benh nhan co khoa khong hop le: "
                      << bn.khoa << "\n";
            continue;
        }

        HangDoiTheoKhoa[index].push(bn);
        ++soNap;
    }

    std::cout << "Da nap " << soNap
              << " benh nhan vao cac hang doi RAM.\n";
}

int QuanLyKhamBenh::RandomThoiGianKham() const {
    static std::mt19937 gen(
        static_cast<unsigned int>(
            std::chrono::system_clock::now().time_since_epoch().count()
        )
    );

    std::uniform_int_distribution<int> dist(10, 30);
    return dist(gen);
}

void QuanLyKhamBenh::InDanhSachBacSiTheoKhoa(
    const std::string& khoa
) const {
    std::vector<int> danhSach =
        QuanLyBacSiManager.LayBacSiTheoKhoa(khoa);

    std::cout << "\n===== " << tenKhoaDep(khoa) << " =====\n";

    if (danhSach.empty()) {
        std::cout << "Khong co bac si nao trong khoa.\n";
        return;
    }

    for (int index : danhSach) {
        const BacSi& bs = QuanLyBacSiManager.LayBacSi(index);

        std::cout
            << bs.id << " | "
            << bs.name
            << " | Kinh nghiem: "
            << bs.ExpYears << " nam\n";
    }
}

bool QuanLyKhamBenh::TimBacSiTotNhat(
    const std::string& khoa,
    std::time_t hienTai,
    int thoiLuong,
    int& bacSiIndex,
    std::time_t& batDau
) {
    // MinHeap tu cai dat: phan tu dau la bac si co thoi diem bat dau som nhat.
    MinHeapTuCaiDat<UngVienBacSi, SoSanhUngVien> minHeap;

    std::vector<int> danhSach =
        QuanLyBacSiManager.LayBacSiTheoKhoa(khoa);

    for (int index : danhSach) {
        std::time_t thoiDiemNhan = 0;

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
    const std::string& khoaThucTe,
    std::time_t hienTai
) {
    int thoiLuong = RandomThoiGianKham();

    int bacSiIndex = -1;
    std::time_t batDau = 0;

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
    std::time_t ketThuc = batDau + thoiLuong * 60;

    if (khoaThucTe != "Khoa Cap cuu" &&
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

void QuanLyKhamBenh::XuLyKhoa(
    const std::string& khoa
) {
    int khoaIndex = TimChiSoKhoa(khoa);
    if (khoaIndex < 0) {
        std::cout << "Khoa khong hop le.\n";
        return;
    }

    std::time_t hienTai = ThoiGian::HienTai();

    std::cout << "\n========================================\n";
    std::cout << "XU LY " << tenKhoaDep(khoa) << "\n";
    std::cout << "Thoi gian hien tai: "
              << ThoiGian::DinhDang(hienTai) << "\n";

    // Mo phong 8% bac si ban dot xuat chi mot lan cho ca phien chay.
    if (!DaMoPhongBanDotXuat) {
        QuanLyBacSiManager.MoPhongBanDotXuat(hienTai);
        DaMoPhongBanDotXuat = true;
    }

    HangDoiTuCaiDat<BenhNhanKham>& hangDoi =
        HangDoiTheoKhoa[khoaIndex];

    if (hangDoi.empty()) {
        std::cout << "Hang doi dang rong.\n";
        return;
    }

    std::cout << "So benh nhan trong hang: "
              << hangDoi.size() << "\n";

    std::vector<BenhNhanKham> ketQuaKhoa;

    // Cuoi tuan: khoa thuong nghi -> chuyen BN sang cap cuu.
    if (khoa != "Khoa Cap cuu" &&
        ThoiGian::LaCuoiTuan(hienTai)) {

        std::cout
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

                std::cout
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

                std::cout
                    << "\nBN checkin_id = "
                    << bn.CheckinId
                    << " chua xep duoc bac si.\n";

                // Bao toan BN va tranh lap vo han.
                hangDoi.push(bn);
                break;
            }
        }

        std::cout
            << "\nTong ket: "
            << soDaXuLy << " benh nhan da xep, "
            << soCho << " benh nhan dang cho.\n";
    }

    if (!ketQuaKhoa.empty()) {
        if (Database.GhiKetQuaNhieu(ketQuaKhoa)) {
            std::cout << "Da ghi " << ketQuaKhoa.size()
                      << " ket qua vao database.\n";
        } else {
            std::cout << "Ghi ket qua vao database that bai.\n";
        }
    }
}

void QuanLyKhamBenh::XuLyTatCaKhoa() {
    for (int i = 0; i < SO_KHOA; ++i) {
        XuLyKhoa(CAC_KHOA[i]);
    }
}

void QuanLyKhamBenh::HienThiKetQua() const {
    std::cout << "\n===== KET QUA TRONG PHIEN CHAY =====\n";

    if (KetQuaTrongLanChay.empty()) {
        std::cout << "Chua co ket qua.\n";
        return;
    }

    for (const BenhNhanKham& bn : KetQuaTrongLanChay) {
        std::cout
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
