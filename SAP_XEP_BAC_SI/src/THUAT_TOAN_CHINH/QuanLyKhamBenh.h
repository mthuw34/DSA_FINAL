#pragma once
#include "BenhNhanKham.h"
#include "CauTrucDuLieu.h"
#include "QuanLyBacSi.h"
#include "XuLyDatabase.h"
#include <string>
#include <vector>
#include <ctime>
using namespace std;

class QuanLyKhamBenh {
private:
    static const int SO_KHOA = 10;

    QuanLyBacSi QuanLyBacSiManager;
    XuLyDatabase Database;

    HangDoiTuCaiDat<BenhNhanKham> HangDoiTheoKhoa[SO_KHOA];

    vector<BenhNhanKham> KetQuaTrongLanChay;
    bool DaMoPhongBanDotXuat = false;

    int TimChiSoKhoa(const string& khoa) const;
    void NapHangDoiTuDatabase();
    int RandomThoiGianKham() const;

    bool XuLyMotBenhNhan(
        BenhNhanKham& benhNhan,
        const string& khoaThucTe,
        time_t HienTai
    );

    bool TimBacSiTotNhat(
        const string& khoa,
        time_t HienTai,
        int thoiLuong,
        int& bacSiIndex,
        time_t& batDau
    );

public:
    bool KhoiDong(
        const string& duongDanDatabase,
        const string& duongDanCSV
    );

    void InDanhSachBacSiTheoKhoa(const string& khoa) const;

    void XuLyKhoa(const string& khoa);

    void XuLyTatCaKhoa();

    void HienThiKetQua() const;
};