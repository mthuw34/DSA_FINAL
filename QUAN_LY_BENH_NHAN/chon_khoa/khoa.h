#ifndef KHOA_H
#define KHOA_H

#include <string>

std::string chonKhoa();

bool khoaDangHoatDong(
    const std::string& department,
    int priority,
    std::string& lyDo
);

#endif