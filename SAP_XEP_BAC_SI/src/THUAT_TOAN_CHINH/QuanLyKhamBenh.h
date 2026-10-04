#pragma once
#include "BenhNhanKham.h"
#include "CauTrucDuLieu.h"
#include "QuanLyBacSi.h"
#include "XuLyDatabase.h"
#include <string>
#include <vector>
#include <ctime>

class QuanLyKhamBenh {
private:
    static const int SO_KHOA = 10;

    QuanLyBacSi QuanLyBacSiManager;
    XuLyDatabase Database;

    // Hang doi cua tung khoa da duoc nap vao RAM ngay luc khoi dong.
    HangDoiTuCaiDat<BenhNhanKham> HangDoiTheoKhoa[SO_KHOA];

    std::vector<BenhNhanKham> KetQuaTrongLanChay;
    bool DaMoPhongBanDotXuat = false;

    int TimChiSoKhoa(const std::string& khoa) const;
    void NapHangDoiTuDatabase();
    int RandomThoiGianKham() const;

    bool XuLyMotBenhNhan(
        BenhNhanKham& benhNhan,
        const std::string& khoaThucTe,
        std::time_t HienTai
    );

    bool TimBacSiTotNhat(
        const std::string& khoa,
        std::time_t HienTai,
        int thoiLuong,
        int& bacSiIndex,
        std::time_t& batDau
    );

public:
    bool KhoiDong(
        const std::string& duongDanDatabase,
        const std::string& duongDanCSV
    );

    void InDanhSachBacSiTheoKhoa(const std::string& khoa) const;

    void XuLyKhoa(const std::string& khoa);

    void XuLyTatCaKhoa();

    void HienThiKetQua() const;
};
