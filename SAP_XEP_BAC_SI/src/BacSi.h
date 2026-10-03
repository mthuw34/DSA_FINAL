#pragma once
#include <string>
#include "HangDoiBacSi.h"

using namespace std;

struct BacSi
{
    // Mã bác sĩ do chương trình tự tạo
    // Ví dụ: BS001, BS002,...
    string MaBacSi;

    // Tên bác sĩ
    string TenBacSi;

    // Khoa chuyên môn
    // Ví dụ: khoa_cap_cuu
    string TenKhoa;

    // Số bệnh nhân đang chờ
    int SoBenhNhanDangCho = 0;

    // Hàng đợi bệnh nhân của bác sĩ
    HangDoiBacSi DanhSachCho;
};