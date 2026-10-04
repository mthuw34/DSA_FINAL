#include "QuanLyBacSi.h"
#include "ThoiGian.h"
#include <fstream>
#include <iostream>
#include <random>
#include <chrono>

namespace {
    std::vector<std::string> tachCSV(const std::string& line) {
        std::vector<std::string> result;
        std::string current;
        bool trongNgoac = false;

        for (char c : line) {
            if (c == '"') {
                trongNgoac = !trongNgoac;
            } else if (c == ',' && !trongNgoac) {
                result.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }

        result.push_back(current);
        return result;
    }

    int toInt(const std::string& s) {
        try {
            return std::stoi(s);
        } catch (...) {
            return 0;
        }
    }
}

bool QuanLyBacSi::DocCSV(const std::string& duongDan) {
    std::ifstream file(duongDan);
    if (!file.is_open()) {
        std::cerr << "Khong mo duoc file bac si: " << duongDan << "\n";
        return false;
    }

    DanhSach.clear();

    std::string line;
    bool dongDau = true;
    int stt = 1;

    while (std::getline(file, line)) {
        // CSV UTF-8 từ Excel có thể có BOM; getline trên Windows cũng có thể giữ CR.
        if (dongDau && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::vector<std::string> cot = tachCSV(line);

        if (dongDau) {
            dongDau = false;
            if (!cot.empty() && cot[0] == "name") continue;
        }

        if (cot.size() < 9) {
            std::cerr << "Bo qua dong CSV khong hop le.\n";
            continue;
        }

        BacSi bs;
        bs.id = "BS";
        if (stt < 10) bs.id += "00";
        else if (stt < 100) bs.id += "0";
        bs.id += std::to_string(stt);

        bs.name = cot[0];
        bs.BirthDay = cot[1];
        bs.age = toInt(cot[2]);
        bs.gender = cot[3];
        bs.hometown = cot[4];
        bs.address = cot[5];
        bs.phone = cot[6];
        bs.ExpYears = toInt(cot[7]);

        // CSV da dung cung dinh dang voi database cua ban:
        // "Khoa Noi", "Khoa Ngoai", ...
        bs.khoaChuyenMon = cot[8];

        DanhSach.push_back(bs);
        ++stt;
    }

    file.close();

    std::cout << "Da doc " << DanhSach.size() << " bac si.\n";
    return !DanhSach.empty();
}

const std::vector<BacSi>& QuanLyBacSi::LayDanhSach() const {
    return DanhSach;
}

std::vector<int> QuanLyBacSi::LayBacSiTheoKhoa(
    const std::string& khoa
) const {
    std::vector<int> result;

    for (int i = 0; i < static_cast<int>(DanhSach.size()); ++i) {
        if (DanhSach[i].khoaChuyenMon == khoa) {
            result.push_back(i);
        }
    }

    return result;
}

void QuanLyBacSi::KhoiTaoLich(std::time_t hienTai) {
    NgayBatDauTrucCapCuu.assign(DanhSach.size(), 0);

    std::mt19937 gen(
        static_cast<unsigned int>(
            std::chrono::system_clock::now().time_since_epoch().count()
        )
    );
    std::uniform_int_distribution<int> randomPha(0, 1);

    for (int i = 0; i < static_cast<int>(DanhSach.size()); ++i) {
        if (DanhSach[i].khoaChuyenMon == "Khoa Cap cuu") {
            NgayBatDauTrucCapCuu[i] = randomPha(gen);
        }

        DanhSach[i].TamNghiDen = 0;

        if (DanhSach[i].khoaChuyenMon == "Khoa Cap cuu") {
            DanhSach[i].ThoiGianRanh =
                ThoiGian::TrucCapCuuTiepTheo(
                    hienTai,
                    NgayBatDauTrucCapCuu[i]
                );
            DanhSach[i].DangLamViec =
                ThoiGian::DangTrucCapCuu(
                    hienTai,
                    NgayBatDauTrucCapCuu[i]
                );
        } else {
            DanhSach[i].ThoiGianRanh =
                ThoiGian::DieuChinhThoiGianKhoaThuong(hienTai);
            DanhSach[i].DangLamViec =
                ThoiGian::DangTrongCaThuong(hienTai);
        }
    }
}

bool QuanLyBacSi::CoBacSiDangTruc(
    const std::string& khoa,
    std::time_t thoiDiem
) const {
    for (int i : LayBacSiTheoKhoa(khoa)) {
        bool dangTruc = false;

        if (khoa == "Khoa Cap cuu") {
            dangTruc = ThoiGian::DangTrucCapCuu(
                thoiDiem,
                NgayBatDauTrucCapCuu[i]
            );
        } else {
            dangTruc = ThoiGian::DangTrongCaThuong(thoiDiem);
        }

        if (dangTruc) return true;
    }

    return false;
}

bool QuanLyBacSi::TinhThoiDiemNhanBenhNhan(
    int index,
    const std::string& khoa,
    std::time_t hienTai,
    int thoiLuong,
    std::time_t& batDau
) const {
    if (index < 0 || index >= static_cast<int>(DanhSach.size())) {
        return false;
    }

    const BacSi& bs = DanhSach[index];

    std::time_t t = hienTai;
    if (bs.ThoiGianRanh > t) t = bs.ThoiGianRanh;
    if (bs.TamNghiDen > t) t = bs.TamNghiDen;

    if (khoa == "Khoa Cap cuu") {
        if (!ThoiGian::DangTrucCapCuu(
                t,
                NgayBatDauTrucCapCuu[index])) {
            t = ThoiGian::TrucCapCuuTiepTheo(
                t,
                NgayBatDauTrucCapCuu[index]
            );
        }

        batDau = t;
        return true;
    }

    while (true) {
        t = ThoiGian::DieuChinhThoiGianKhoaThuong(t);

        if (ThoiGian::DuThoiGianKhamKhoaThuong(t, thoiLuong)) {
            batDau = t;
            return true;
        }

        t = ThoiGian::CaThuongTiepTheo(t + 60);
    }
}

void QuanLyBacSi::CapNhatSauKhiKham(
    int index,
    std::time_t ketThuc
) {
    if (index < 0 || index >= static_cast<int>(DanhSach.size())) return;
    DanhSach[index].ThoiGianRanh = ketThuc;
}

void QuanLyBacSi::MoPhongBanDotXuat(std::time_t hienTai) {
    std::mt19937 gen(
        static_cast<unsigned int>(
            std::chrono::system_clock::now().time_since_epoch().count()
        )
    );

    std::uniform_int_distribution<int> randomTyLe(1, 100);
    std::uniform_int_distribution<int> randomPhut(20, 45);

    for (BacSi& bs : DanhSach) {
        if (bs.DangLamViec && randomTyLe(gen) <= 8) {
            bs.TamNghiDen = hienTai + randomPhut(gen) * 60;
        }
    }
}

const BacSi& QuanLyBacSi::LayBacSi(int index) const {
    return DanhSach.at(index);
}

BacSi& QuanLyBacSi::LayBacSi(int index) {
    return DanhSach.at(index);
}
