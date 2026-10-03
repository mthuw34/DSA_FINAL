#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include "BacSi.h"
#include "FixedCapacityList.h"

using namespace std;

class ChiaBacSi
{
private:
    // Danh sách tất cả bác sĩ đang trực
    vector<BacSi*> DanhSachBacSiTruc;

    // Tra cứu bệnh nhân đang thuộc bác sĩ nào
    //
    // key   = ID bệnh nhân
    // value = con trỏ đến bác sĩ
    unordered_map<string, BacSi*> BangDinhTuyen;

    // Chuẩn hóa tên khoa
    string ChuanHoaTenKhoa(const string& khoa);

public:
    // Constructor
    ChiaBacSi();

    // Destructor
    ~ChiaBacSi();

    // Đọc danh sách bác sĩ từ CSV
    bool DocDanhSachBacSi(const string& tenFile);

    // Thêm một bác sĩ
    void ThemBacSi(
        const string& maBacSi,
        const string& tenBacSi,
        const string& tenKhoa
    );

    // Phân bệnh nhân cho bác sĩ
    bool PhanBoBenhNhan(const Patient& benhNhan);

    // Chuyển toàn bộ bệnh nhân của một bác sĩ
    // sang bác sĩ khác cùng khoa
    bool ThuyenChuyenBacSi(const string& maBacSiCu);

    // Khám xong bệnh nhân ở đầu Heap
    // và chuyển sang FixedCapacityList
    bool KhamXongVaChuyenBuoc5(
        const string& idBenhNhan,
        FixedCapacityList& danhSachDaKham
    );

    // Phiên bản có cập nhật chẩn đoán + thời gian
    bool KhamXongVaChuyenBuocCuoi(
        const string& idBenhNhan,
        const string& chanDoan,
        const checkoutTime& thoiGian,
        FixedCapacityList& danhSachDaKham
    );

    // Tìm bác sĩ theo mã
    BacSi* TimBacSiTheoMa(const string& maBacSi);

    // Tìm bác sĩ đang giữ bệnh nhân
    BacSi* TimBacSiCuaBenhNhan(const string& idBenhNhan);

    // In danh sách bác sĩ
    void InDanhSachBacSi() const;
};