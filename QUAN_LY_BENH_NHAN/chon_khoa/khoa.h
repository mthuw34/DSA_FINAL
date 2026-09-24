#ifndef KHOA_H
#define KHOA_H

#include <string>

// Tra ve chuoi rong khi dau vao bi dong hoac khong the doc.
std::string chonKhoa();

bool khoaDangHoatDong(
    const std::string& department,
    int priority,
    std::string& lyDo
);

#endif
