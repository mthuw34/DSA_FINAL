#pragma once
#include <ctime>
#include <stdexcept>
#include <string>

inline int parseNonNegativeInt(const std::string& value)
{
    if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("So nguyen khong hop le");
    return std::stoi(value);
}

inline int ageFromBirthDate(const std::string& value)
{
    if (value.size() != 10 || value[4] != '-' || value[7] != '-')
        throw std::invalid_argument("Ngay sinh phai la YYYY-MM-DD");
    const int year = parseNonNegativeInt(value.substr(0, 4));
    const int month = parseNonNegativeInt(value.substr(5, 2));
    const int day = parseNonNegativeInt(value.substr(8, 2));
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    const int days[] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (year < 1 || month < 1 || month > 12 || day < 1 || day > days[month - 1])
        throw std::invalid_argument("Ngay sinh khong hop le");
    const std::time_t now = std::time(nullptr);
    const std::tm* today = std::localtime(&now);
    if (!today) throw std::runtime_error("Khong doc duoc ngay hien tai");
    const int currentYear = today->tm_year + 1900;
    const int currentMonth = today->tm_mon + 1;
    const int age = currentYear - year -
        (currentMonth < month || (currentMonth == month && today->tm_mday < day));
    if (age < 0) throw std::invalid_argument("Ngay sinh o tuong lai");
    return age;
}
