#include "HangDoiBacSi.h"

using namespace std;

// =====================================================
// VUN LÊN
// =====================================================

void HangDoiBacSi::VunLen(int viTri)
{
    while (viTri > 0)
    {
        int cha = (viTri - 1) / 2;

        // Nếu cha đã lớn hơn hoặc bằng con
        // thì Heap đã đúng
        if (MangDuLieu[cha].priority >= MangDuLieu[viTri].priority)
            break;

        // Đổi chỗ
        swap(MangDuLieu[cha], MangDuLieu[viTri]);
        viTri = cha;
    }
}


// =====================================================
// VUN XUỐNG
// =====================================================

void HangDoiBacSi::VunXuong(int viTri)
{
    int n = MangDuLieu.size();

    while (true)
    {
        int trai = 2 * viTri + 1;
        int phai = 2 * viTri + 2;
        int lonNhat = viTri;

        // So sánh với con trái
        if (trai < n && MangDuLieu[trai].priority > MangDuLieu[lonNhat].priority)
            lonNhat = trai;

        // So sánh với con phải
        if (phai < n && MangDuLieu[phai].priority > MangDuLieu[lonNhat].priority)
            lonNhat = phai;

        // Không còn con nào lớn hơn
        if (lonNhat == viTri)
            break;

        swap(MangDuLieu[viTri], MangDuLieu[lonNhat]);

        viTri = lonNhat;
    }
}


// =====================================================
// THÊM BỆNH NHÂN
// =====================================================

void HangDoiBacSi::ThemBenhNhan(const Patient& benhNhan)
{
    MangDuLieu.push_back(benhNhan);

    // Node mới nằm ở cuối -> vun lên để khôi phục Max-Heap
    VunLen((int)MangDuLieu.size() - 1);
}


// =====================================================
// NỐI MỘT MẢNG BỆNH NHÂN
// =====================================================

void HangDoiBacSi::NoiMangCapToc(const vector<Patient>& danhSach)
{
    for (const Patient& benhNhan : danhSach)
    {
        MangDuLieu.push_back(benhNhan);
    }
}


// =====================================================
// PHỤC HỒI HEAP
// =====================================================

void HangDoiBacSi::PhucHoiHeap()
{
    int n = MangDuLieu.size();

    // Bắt đầu từ node cha cuối cùng
    for (int i = n / 2 - 1; i >= 0; i--)
    {
        VunXuong(i);
    }
}


// =====================================================
// LẤY BỆNH NHÂN ƯU TIÊN CAO NHẤT
// =====================================================

bool HangDoiBacSi::LayBenhNhanUuTienNhat(Patient& benhNhan)
{
    if (MangDuLieu.empty())
    {
        return false;
    }

    // Phần tử đầu tiên của Max-Heap
    // luôn là bệnh nhân có priority cao nhất
    benhNhan = MangDuLieu[0];

    // Đưa phần tử cuối lên đầu
    MangDuLieu[0] = MangDuLieu.back();

    // Xóa phần tử cuối
    MangDuLieu.pop_back();

    // Nếu còn phần tử -> phục hồi Heap
    if (!MangDuLieu.empty())
    {
        VunXuong(0);
    }

    return true;
}


// =====================================================
// XÓA TOÀN BỘ
// =====================================================

void HangDoiBacSi::LamSach()
{
    MangDuLieu.clear();
}


// =====================================================
// LẤY SỐ LƯỢNG
// =====================================================

int HangDoiBacSi::SoLuong() const
{
    return (int)MangDuLieu.size();
}