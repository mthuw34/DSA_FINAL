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

    for (int i = 0; i < static_cast<int>(DanhSach.size()); ++i) {
        BacSi& bs = DanhSach[i];
        bs.TamNghiDen = 0;
        bs.NghiSauCaDemDen = 0;
        bs.TrucThuCong = false;

        if (bs.khoaChuyenMon == "Khoa Cap cuu") {
            // 0 = ca ngay, 1 = ca dem. Phan co dinh de Web/CLI dong nhat.
            NgayBatDauTrucCapCuu[i] = i % 2;
            bs.ThoiGianRanh = ThoiGian::TrucCapCuuTiepTheo(
                hienTai, NgayBatDauTrucCapCuu[i]);
            bs.DangLamViec = ThoiGian::DangTrucCapCuu(
                hienTai, NgayBatDauTrucCapCuu[i]);
        } else {
            bs.ThoiGianRanh = ThoiGian::DieuChinhThoiGianKhoaThuong(hienTai);
            bs.DangLamViec = ThoiGian::DangTrongCaThuong(hienTai);
        }
    }
}

int QuanLyBacSi::LayPhaTrucCapCuu(int index) const {
    if (index < 0 || index >= static_cast<int>(DanhSach.size())) return -1;
    return NgayBatDauTrucCapCuu.empty() ? (index % 2) : NgayBatDauTrucCapCuu[index];
}

bool QuanLyBacSi::DangTrucBacSi(int index, time_t thoiDiem) const {
    if (index < 0 || index >= static_cast<int>(DanhSach.size())) return false;
    if (DanhSach[index].khoaChuyenMon == "Khoa Cap cuu") {
        return ThoiGian::DangTrucCapCuu(thoiDiem, LayPhaTrucCapCuu(index));
    }
    return ThoiGian::DangTrongCaThuong(thoiDiem);
}

bool QuanLyBacSi::CoBacSiDangTruc(
    const string& khoa,
    time_t thoiDiem
) const {
    for (int i : LayBacSiTheoKhoa(khoa)) {
        bool dangTruc = false;

        if (khoa == "Khoa Cap cuu") {
            dangTruc = DangTrucBacSi(i, thoiDiem);
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
    if (index < 0 || index >= static_cast<int>(DanhSach.size())) return false;

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
        if (!ThoiGian::DangTrucCapCuu(t, NgayBatDauTrucCapCuu[index])) {
            t = ThoiGian::TrucCapCuuTiepTheo(t, NgayBatDauTrucCapCuu[index]);
        }

        // Khong cho mot ca kham bi tran qua moc ket thuc ca truc.
        const tm local = [&]() {
            tm x{};
#ifdef _WIN32
            localtime_s(&x, &t);
#else
            localtime_r(&t, &x);
#endif
            return x;
        }();
        const int p = local.tm_hour * 60 + local.tm_min;
        const int ketThuc = p + thoiLuong;
        const bool laCaNgay = NgayBatDauTrucCapCuu[index] == 0;
        if ((laCaNgay && ketThuc > 18 * 60) ||
            (!laCaNgay && p < 6 * 60 && ketThuc > 6 * 60) ||
            (!laCaNgay && p >= 18 * 60 && ketThuc > 24 * 60)) {
            t = ThoiGian::TrucCapCuuTiepTheo(t + 60, NgayBatDauTrucCapCuu[index]);
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

    BacSi& bs = DanhSach[index];
    bs.ThoiGianRanh = ketThuc;

    if (bs.khoaChuyenMon == "Khoa Cap cuu" &&
        NgayBatDauTrucCapCuu[index] == 1) {
        tm local{};
#ifdef _WIN32
        localtime_s(&local, &ketThuc);
#else
        localtime_r(&ketThuc, &local);
#endif
        const int p = local.tm_hour * 60 + local.tm_min;
        if (p >= 5 * 60 && p <= 7 * 60) {
            local.tm_hour = 6;
            local.tm_min = 0;
            local.tm_sec = 0;
            local.tm_mday += 2;
            bs.NghiSauCaDemDen = mktime(&local);
        }
    }
}

const BacSi& QuanLyBacSi::LayBacSi(int index) const {
    return DanhSach.at(index);
}

BacSi& QuanLyBacSi::LayBacSi(int index) {
    return DanhSach.at(index);
}
