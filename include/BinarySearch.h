#pragma once
#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace DsaSearch {
// Chi muc (ID, vi tri goc): sap xep mot lan, tim nhi phan O(log n).
// Khong thay doi thu tu cua vector ban ghi dau vao.
class BinaryIdIndex {
    std::vector<std::pair<int, std::size_t>> entries;
public:
    template<class T, class Key>
    BinaryIdIndex(const std::vector<T>& records, Key key) {
        entries.reserve(records.size());
        for (std::size_t i = 0; i < records.size(); ++i)
            entries.emplace_back(key(records[i]), i);
        std::sort(entries.begin(), entries.end());
    }

    bool find(int id, std::size_t& position) const {
        std::size_t left = 0, right = entries.size();
        while (left < right) {
            const std::size_t middle = left + (right - left) / 2;
            if (entries[middle].first < id) left = middle + 1;
            else right = middle;
        }
        if (left == entries.size() || entries[left].first != id) return false;
        position = entries[left].second;
        return true;
    }
};
}
