#pragma once
#include <stdexcept>
#include "TruyXuat.h"

// Precondition: Đây là Cấu trúc dữ liệu tự cài đặt từ đầu (Dynamic Array) 
// theo đúng giáo trình (Chương 5: Arrays, Dynamic Arrays), thay thế cho std::vector.
class MangDongBenhNhan {
private:
    HoSoTruyXuat* data; // The underlying fixed-size block[cite: 33]
    int m_size;         // Elements actually stored[cite: 33]
    int m_capacity;     // Slots currently allocated[cite: 33]

    // Hàm nhân đôi sức chứa khi mảng đầy (O(n) nhưng amortized O(1))[cite: 33]
    void resize(int new_capacity) {
        HoSoTruyXuat* new_data = new HoSoTruyXuat[new_capacity];
        for (int i = 0; i < m_size; ++i) {
            new_data[i] = data[i]; // Copy only live elements[cite: 33]
        }
        delete[] data;
        data = new_data;
        m_capacity = new_capacity;
    }

public:
    MangDongBenhNhan() : m_size(0), m_capacity(1) { // Initialize capacity to at least 1[cite: 33]
        data = new HoSoTruyXuat[m_capacity];
    }

    MangDongBenhNhan(const MangDongBenhNhan&) = delete;
    MangDongBenhNhan& operator=(const MangDongBenhNhan&) = delete;

    ~MangDongBenhNhan() {
        delete[] data;
    }

    // Cost: O(1) amortized[cite: 33]
    void push_back(const HoSoTruyXuat& value) {
        if (m_size == m_capacity) {
            resize(m_capacity * 2); // Doubling is the whole trick[cite: 33]
        }
        data[m_size] = value;
        m_size++;
    }

    // Cost: O(n) shifts[cite: 33]
    void erase(int index) {
        if (index < 0 || index >= m_size) {
            throw std::out_of_range("Index out of bounds");
        }
        // Deletion mirrors insertion - survivors close the gap by shifting left[cite: 33]
        for (int i = index; i < m_size - 1; ++i) {
            data[i] = data[i + 1]; 
        }
        m_size--;
    }

    void clear() {
        m_size = 0; // Capacity deliberately unchanged[cite: 33]
    }

    int size() const { return m_size; }
    bool empty() const { return m_size == 0; }

    HoSoTruyXuat& operator[](int index) { return data[index]; }
    const HoSoTruyXuat& operator[](int index) const { return data[index]; }
};