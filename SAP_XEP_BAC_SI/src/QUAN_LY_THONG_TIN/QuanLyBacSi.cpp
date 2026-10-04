#include "QuanLyBacSi.h"
#include "ThoiGian.h"
#include <fstream>
#include <iostream>
#include <random>
#include <chrono>
using namespace std;

namespace {
    vector<string> tachCSV(const string& line) {
        vector<string> result;
        string current;
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

    int toInt(const string& s) {
        try {
            return stoi(s);
        } catch (...) {
            return 0;
        }
    }
}

bool QuanLyBacSi::DocCSV(const string& duongDan) {
    ifstream file(duongDan);
    if (!file.is_open()) {
        cerr << "Khong mo duoc file bac si: " << duongDan << "\n";
        return false;
    }

    DanhSach.clear();

    string line;
    bool dongDau = true;
    int stt = 1;

    while (getline(file, line)) {
        if (line.empty()) continue;

        vector<string> cot = tachCSV(line);

        if (dongDau) {
            dongDau = false;
            if (!cot.empty() && cot[0] == "name") continue;
        }

        if (cot.size() < 9) {
            cerr << "Bo qua dong CSV khong hop le.\n";
            continue;
        }

        BacSi bs;
        bs.id = "BS";
        if (stt < 10) bs.id += "00";
        else if (stt < 100) bs.id += "0";
        bs.id += to_string(stt);

        bs.name = cot[0];
        bs.BirthDay = cot[1];
        bs.age = toInt(cot[2]);
        bs.gender = cot[3];
        bs.hometown = cot[4];
        bs.address = cot[5];
        bs.phone = cot[6];
        bs.ExpYears = toInt(cot[7]);

        // Đã đúng định dạng
        bs.khoaChuyenMon = cot[8];

        DanhSach.push_back(bs);
        ++stt;
    }

    file.close();

    cout << "Da doc " << DanhSach.size() << " bac si.\n";
    return !DanhSach.empty();
}

const vector<BacSi>& QuanLyBacSi::LayDanhSach() const {
    return DanhSach;
}

vector<int> QuanLyBacSi::LayBacSiTheoKhoa(
    const string& khoa
) const {
    vector<int> result;

    for (int i = 0; i < static_cast<int>(DanhSach.size()); ++i) {
        if (DanhSach[i].khoaChuyenMon == khoa) {
            result.push_back(i);
        }
    }

    return result;
}

void QuanLyBacSi::KhoiTaoLich(time_t hienTai) {
    NgayBatDauTrucCapCuu.assign(DanhSach.size(), 0);

    mt19937 gen(
        static_cast<unsigned int>(
            chrono::system_clock::now().time_since_epoch().count()
        )
    );
    uniform_int_distribution<int> randomPha(0, 1);

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
    const string& khoa,
    time_t thoiDiem
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
    const string& khoa,
    time_t hienTai,
    int thoiLuong,
    time_t& batDau
) const {
    if (index < 0 || index >= static_cast<int>(DanhSach.size())) {
        return false;
    }

    const BacSi& bs = DanhSach[index];

    time_t t = hienTai;
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
    time_t ketThuc
) {
    if (index < 0 || index >= static_cast<int>(DanhSach.size())) return;
    DanhSach[index].ThoiGianRanh = ketThuc;
}

void QuanLyBacSi::MoPhongBanDotXuat(time_t hienTai) {
    mt19937 gen(
        static_cast<unsigned int>(
            chrono::system_clock::now().time_since_epoch().count()
        )
    );

    uniform_int_distribution<int> randomTyLe(1, 100);
    uniform_int_distribution<int> randomPhut(20, 45);

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