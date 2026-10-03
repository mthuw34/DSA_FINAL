#pragma once
#include "BacSi.h"
#include <string>
#include <vector>
#include <ctime>

struct BacSiLich {
    int index = -1;
    std::time_t thoiDiemNhan = 0;
};

class QuanLyBacSi {
private:
    std::vector<BacSi> DanhSach;
    std::vector<int> NgayBatDauTrucCapCuu;

public:
    bool DocCSV(const std::string& duongDan);

    const std::vector<BacSi>& LayDanhSach() const;

    std::vector<int> LayBacSiTheoKhoa(const std::string& khoa) const;

    void KhoiTaoLich(std::time_t HienTai);

    bool CoBacSiDangTruc(const std::string& khoa, std::time_t thoiDiem) const;

    bool TinhThoiDiemNhanBenhNhan(
        int index,
        const std::string& khoa,
        std::time_t HienTai,
        int thoiLuong,
        std::time_t& batDau
    ) const;

    void CapNhatSauKhiKham(int index, std::time_t ketThuc);

    void MoPhongBanDotXuat(std::time_t HienTai);

    const BacSi& LayBacSi(int index) const;
    BacSi& LayBacSi(int index);
};
