#pragma once
#include <vector>
#include "patient.h"

using namespace std;

class HangDoiBacSi
{
public:
    // Mảng lưu bệnh nhân
    vector<Patient> MangDuLieu;

    // Thêm bệnh nhân vào Heap
    void ThemBenhNhan(const Patient& benhNhan);

    // Trộn thêm một mảng bệnh nhân vào Heap
    void NoiMangCapToc(const vector<Patient>& danhSach);

    // Xây lại Max-Heap từ đầu
    void PhucHoiHeap();

    // Lấy bệnh nhân ưu tiên cao nhất ra khỏi Heap
    bool LayBenhNhanUuTienNhat(Patient& benhNhan);

    // Xóa toàn bộ bệnh nhân
    void LamSach();

    // Số lượng bệnh nhân
    int SoLuong() const;

private:
    // Đưa node lên trên
    void VunLen(int viTri);

    // Đưa node xuống dưới
    void VunXuong(int viTri);
};