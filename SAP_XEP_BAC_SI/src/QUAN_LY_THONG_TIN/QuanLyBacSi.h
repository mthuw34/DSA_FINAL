#pragma once
#include "BacSi.h"
#include <string>
#include <vector>
#include <ctime>
using namespace std;

struct BacSiLich {
    int index = -1;
    time_t thoiDiemNhan = 0;
};

class QuanLyBacSi {
private:
    vector<BacSi> DanhSach;
    vector<int> NgayBatDauTrucCapCuu;

public:
    bool DocCSV(const string& duongDan);

    const vector<BacSi>& LayDanhSach() const;

    vector<int> LayBacSiTheoKhoa(const string& khoa) const;

    void KhoiTaoLich(time_t HienTai);

    // 0 = ca ngay, 1 = ca dem. Web dung chung pha nay voi bo phan xep lich.
    int LayPhaTrucCapCuu(int index) const;

    bool DangTrucBacSi(int index, time_t thoiDiem) const;

    bool CoBacSiDangTruc(const string& khoa, time_t thoiDiem) const;

    bool TinhThoiDiemNhanBenhNhan(
        int index,
        const string& khoa,
        time_t HienTai,
        int thoiLuong,
        time_t& batDau
    ) const;

    void CapNhatSauKhiKham(int index, time_t ketThuc);


    const BacSi& LayBacSi(int index) const;
    BacSi& LayBacSi(int index);
};
