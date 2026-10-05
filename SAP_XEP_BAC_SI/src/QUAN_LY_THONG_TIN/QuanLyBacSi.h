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

    // Lưu pha trực Cấp cứu 0..5 của từng bác sĩ.
    vector<int> NgayBatDauTrucCapCuu;

public:
    bool DocCSV(const string& duongDan);

    const vector<BacSi>& LayDanhSach() const;

    vector<int> LayBacSiTheoKhoa(const string& khoa) const;

    // onDinh giữ lại để test và Web/CLI dùng cùng một cách chia pha.
    void KhoiTaoLich(time_t HienTai, bool onDinh = true);

    // Pha 0..5:
    // 0,1 = 00:00-08:00; 2,3 = 08:00-16:00; 4,5 = 16:00-00:00.
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
