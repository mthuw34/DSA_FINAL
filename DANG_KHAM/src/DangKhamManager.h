#pragma once

#include <string>

#include "DatabaseDangKham.h"

class DangKhamManager
{
private:
    DatabaseDangKham database;

    void hienThiHangDoi();
    void batDauKham();
    void hienThiDangKham();
    void nhapChanDoan();
    void ketThucKham();
    void hienThiLichSu();

    bool layBenhNhanTuHangDoi(
        int checkinId,
        int& patientId,
        std::string& department,
        std::string& checkinTime
    );

    bool daCoPhienKham(int checkinId);

    bool layPhienDangKham(
        int checkinId,
        int& patientId
    );

public:
    bool khoiDong(
        const std::string& sourcePath,
        const std::string& destinationPath
    );

    void chayMenu();
};
