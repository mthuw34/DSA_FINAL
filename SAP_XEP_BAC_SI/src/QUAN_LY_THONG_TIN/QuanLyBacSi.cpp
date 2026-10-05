#include "QuanLyBacSi.h"
#include "ThoiGian.h"
#include <fstream>
#include <iostream>
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
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (dongDau && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
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
        bs.khoaChuyenMon = cot[8];

        DanhSach.push_back(bs);
        ++stt;
    }

    cout << "Da doc " << DanhSach.size() << " bac si.\n";
    return !DanhSach.empty();
}

const vector<BacSi>& QuanLyBacSi::LayDanhSach() const {
    return DanhSach;
}

vector<int> QuanLyBacSi::LayBacSiTheoKhoa(const string& khoa) const {
    vector<int> result;

    for (int i = 0; i < static_cast<int>(DanhSach.size()); ++i)
        if (DanhSach[i].khoaChuyenMon == khoa) result.push_back(i);

    return result;
}

void QuanLyBacSi::KhoiTaoLich(time_t hienTai, bool onDinh) {
    (void)onDinh;
    NgayBatDauTrucCapCuu.assign(DanhSach.size(), 0);

    int capCuuIndex = 0;

    for (int i = 0; i < static_cast<int>(DanhSach.size()); ++i) {
        BacSi& bs = DanhSach[i];
        bs.TamNghiDen = 0;
        bs.NghiSauCaDemDen = 0;
        bs.TrucThuCong = false;

        if (bs.khoaChuyenMon == "Khoa Cap cuu") {
            // Chia lần lượt vào 6 pha để Web và bộ xếp bác sĩ dùng cùng lịch.
            const int pha = capCuuIndex++ % 6;
            NgayBatDauTrucCapCuu[i] = pha;
            bs.ThoiGianRanh = ThoiGian::TrucCapCuuTiepTheo(hienTai, pha);
            bs.DangLamViec = ThoiGian::DangTrucCapCuu(hienTai, pha);
        } else {
            bs.ThoiGianRanh = ThoiGian::DieuChinhThoiGianKhoaThuong(hienTai);
            bs.DangLamViec = ThoiGian::DangTrongCaThuong(hienTai);
        }
    }
}

int QuanLyBacSi::LayPhaTrucCapCuu(int index) const {
    if (index < 0 || index >= static_cast<int>(DanhSach.size())) return -1;
    return NgayBatDauTrucCapCuu.empty() ? 0 : NgayBatDauTrucCapCuu[index];
}

bool QuanLyBacSi::DangTrucBacSi(int index, time_t thoiDiem) const {
    if (index < 0 || index >= static_cast<int>(DanhSach.size())) return false;

    if (DanhSach[index].khoaChuyenMon == "Khoa Cap cuu")
        return ThoiGian::DangTrucCapCuu(thoiDiem, LayPhaTrucCapCuu(index));

    return ThoiGian::DangTrongCaThuong(thoiDiem);
}

bool QuanLyBacSi::CoBacSiDangTruc(
    const string& khoa,
    time_t thoiDiem
) const {
    for (int i : LayBacSiTheoKhoa(khoa)) {
        const bool dangTruc =
            khoa == "Khoa Cap cuu"
                ? DangTrucBacSi(i, thoiDiem)
                : ThoiGian::DangTrongCaThuong(thoiDiem);

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
    if (index < 0 || index >= static_cast<int>(DanhSach.size()) || thoiLuong <= 0)
        return false;

    const BacSi& bs = DanhSach[index];
    time_t t = hienTai;

    if (bs.ThoiGianRanh > t) t = bs.ThoiGianRanh;
    if (bs.TamNghiDen > t) t = bs.TamNghiDen;
    if (bs.NghiSauCaDemDen > t) t = bs.NghiSauCaDemDen;

    if (bs.TrucThuCong) {
        batDau = t;
        return true;
    }

    if (khoa == "Khoa Cap cuu") {
        if (thoiLuong > 8 * 60) return false;

        const int pha = LayPhaTrucCapCuu(index);
        if (pha < 0) return false;

        for (int attempt = 0; attempt < 3; ++attempt) {
            if (!ThoiGian::DangTrucCapCuu(t, pha))
                t = ThoiGian::TrucCapCuuTiepTheo(t, pha);

            if (ThoiGian::DuThoiGianKhamCapCuu(t, thoiLuong, pha)) {
                batDau = t;
                return true;
            }

            // Không đủ thời gian trong ca hiện tại thì chuyển sang ca kế tiếp của nhóm.
            t = ThoiGian::TrucCapCuuTiepTheo(t + 8 * 60 * 60, pha);
        }

        return false;
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

void QuanLyBacSi::CapNhatSauKhiKham(int index, time_t ketThuc) {
    if (index < 0 || index >= static_cast<int>(DanhSach.size())) return;
    DanhSach[index].ThoiGianRanh = ketThuc;
}

const BacSi& QuanLyBacSi::LayBacSi(int index) const {
    return DanhSach.at(index);
}

BacSi& QuanLyBacSi::LayBacSi(int index) {
    return DanhSach.at(index);
}
