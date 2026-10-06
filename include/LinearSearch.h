#pragma once
#include <cstddef>
#include <vector>

namespace DsaSearch {
// Tim mot lan tren du lieu chua sap xep: O(n), O(1) bo nho phu.
// Tra vi tri goc, dung ngay khi gap ban ghi dau tien thoa dieu kien.
template<class T, class Predicate>
bool linearFind(const std::vector<T>& records, Predicate matches, std::size_t& position) {
    for (std::size_t i = 0; i < records.size(); ++i) {
        if (matches(records[i])) { position = i; return true; }
    }
    return false;
}
}
