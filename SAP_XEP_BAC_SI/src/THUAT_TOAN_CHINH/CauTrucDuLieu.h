#pragma once
#include <cstddef>
#include <stdexcept>
#include <utility>

// ============================================================
// Queue tu cai dat bang danh sach lien ket
// ============================================================
template <typename T>
class HangDoiTuCaiDat {
private:
    struct Node {
        T data;
        Node* next;

        explicit Node(const T& value)
            : data(value), next(nullptr) {}
    };

    Node* head = nullptr;
    Node* tail = nullptr;
    std::size_t count = 0;

public:
    HangDoiTuCaiDat() = default;

    HangDoiTuCaiDat(const HangDoiTuCaiDat&) = delete;
    HangDoiTuCaiDat& operator=(const HangDoiTuCaiDat&) = delete;

    HangDoiTuCaiDat(HangDoiTuCaiDat&& other) noexcept
        : head(other.head), tail(other.tail), count(other.count) {
        other.head = nullptr;
        other.tail = nullptr;
        other.count = 0;
    }

    HangDoiTuCaiDat& operator=(HangDoiTuCaiDat&& other) noexcept {
        if (this != &other) {
            clear();
            head = other.head;
            tail = other.tail;
            count = other.count;
            other.head = nullptr;
            other.tail = nullptr;
            other.count = 0;
        }
        return *this;
    }

    ~HangDoiTuCaiDat() {
        clear();
    }

    bool empty() const {
        return head == nullptr;
    }

    std::size_t size() const {
        return count;
    }

    void push(const T& value) {
        Node* node = new Node(value);

        if (tail == nullptr) {
            head = tail = node;
        } else {
            tail->next = node;
            tail = node;
        }

        ++count;
    }

    T& front() {
        if (empty()) {
            throw std::out_of_range("Hang doi rong");
        }
        return head->data;
    }

    const T& front() const {
        if (empty()) {
            throw std::out_of_range("Hang doi rong");
        }
        return head->data;
    }

    void pop() {
        if (empty()) return;

        Node* old = head;
        head = head->next;
        delete old;
        --count;

        if (head == nullptr) {
            tail = nullptr;
        }
    }

    void clear() {
        while (head != nullptr) {
            Node* old = head;
            head = head->next;
            delete old;
        }
        tail = nullptr;
        count = 0;
    }
};

// ============================================================
// MinHeap tu cai dat bang mang dong
// Compare(a,b) == true khi a nen dung truoc b
// ============================================================
template <typename T, typename Compare>
class MinHeapTuCaiDat {
private:
    T* data = nullptr;
    std::size_t count = 0;
    std::size_t capacity = 0;
    Compare compare;

    void resize(std::size_t newCapacity) {
        T* newData = new T[newCapacity];
        for (std::size_t i = 0; i < count; ++i) {
            newData[i] = data[i];
        }
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }

    void siftUp(std::size_t child) {
        while (child > 0) {
            std::size_t parent = (child - 1) / 2;

            if (!compare(data[child], data[parent])) {
                break;
            }

            std::swap(data[child], data[parent]);
            child = parent;
        }
    }

    void siftDown(std::size_t parent) {
        while (true) {
            std::size_t left = parent * 2 + 1;
            std::size_t right = parent * 2 + 2;
            std::size_t best = parent;

            if (left < count && compare(data[left], data[best])) {
                best = left;
            }

            if (right < count && compare(data[right], data[best])) {
                best = right;
            }

            if (best == parent) break;

            std::swap(data[parent], data[best]);
            parent = best;
        }
    }

public:
    explicit MinHeapTuCaiDat(const Compare& cmp = Compare())
        : compare(cmp) {}

    MinHeapTuCaiDat(const MinHeapTuCaiDat&) = delete;
    MinHeapTuCaiDat& operator=(const MinHeapTuCaiDat&) = delete;

    ~MinHeapTuCaiDat() {
        delete[] data;
    }

    bool empty() const {
        return count == 0;
    }

    std::size_t size() const {
        return count;
    }

    void push(const T& value) {
        if (count == capacity) {
            resize(capacity == 0 ? 8 : capacity * 2);
        }

        data[count] = value;
        siftUp(count);
        ++count;
    }

    const T& top() const {
        if (empty()) {
            throw std::out_of_range("MinHeap rong");
        }
        return data[0];
    }

    void pop() {
        if (empty()) return;

        --count;
        if (count == 0) return;

        data[0] = data[count];
        siftDown(0);
    }
};
