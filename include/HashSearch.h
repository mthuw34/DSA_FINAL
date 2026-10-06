#pragma once
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace DsaSearch {
// Ham bam tu cai dat cho ID so va khoa chuoi (ID bac si, khoa import).
inline std::size_t hashKey(int key) { return static_cast<unsigned>(key); }
inline std::size_t hashKey(const std::string& key) {
    std::size_t hash = 2166136261u;
    for (unsigned char c : key) hash = (hash ^ c) * 16777619u;
    return hash;
}

// Separate chaining: moi bucket luu danh sach (khoa, vi tri goc).
// Nap chi muc mot lan de tra cuu nhieu khoa, trung binh O(1) moi lan tim.
template<class Key, std::size_t DefaultBuckets = 4099>
class HashIndex {
    std::vector<std::vector<std::pair<Key, std::size_t>>> buckets;
    std::size_t bucket(const Key& key) const { return hashKey(key) % buckets.size(); }
public:
    explicit HashIndex(std::size_t size = DefaultBuckets) : buckets(size ? size : 1) {}
    template<class T, class ExtractKey>
    HashIndex(const std::vector<T>& records, ExtractKey key) : HashIndex() {
        for (std::size_t i = 0; i < records.size(); ++i) put(key(records[i]), i);
    }
    bool find(const Key& key, std::size_t& position) const {
        for (const auto& entry : buckets[bucket(key)]) {
            if (entry.first == key) { position = entry.second; return true; }
        }
        return false;
    }
    void put(const Key& key, std::size_t position) {
        auto& entries = buckets[bucket(key)];
        for (auto& entry : entries) {
            if (entry.first == key) { entry.second = position; return; }
        }
        entries.emplace_back(key, position);
    }
};
using HashIdIndex = HashIndex<int>;
using HashStringIndex = HashIndex<std::string, 65537>;
}
