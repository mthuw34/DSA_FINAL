#pragma once

#include <string>

#include "DatabaseDangKham.h"

class DangKhamManager
{
private:
    DatabaseDangKham database;

    void hienThiDangKham();
    void nhapChanDoan();
    void ketThucKham();

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
