// Ngăn nội dung tệp tiêu đề được nạp lặp lại.
#pragma once
// Cung cấp ngoại lệ dùng khi truy cập một chỉ số không hợp lệ.
#include <stdexcept>
// Cung cấp định nghĩa hồ sơ bệnh nhân được lưu trong mảng.
#include "TruyXuat.h"

// Mảng động tự cài đặt để lưu các hồ sơ bệnh nhân thay vì dùng std::vector.
class MangDongBenhNhan {
private:
    // Con trỏ tới vùng nhớ liên tục đang lưu các phần tử của mảng.
    HoSoTruyXuat* data;
    // Số phần tử thực tế hiện có trong mảng.
    int m_size;
    // Số vị trí tối đa có thể chứa trước khi cần cấp phát thêm.
    int m_capacity;

    // Cấp phát vùng nhớ mới, sao chép phần tử đang dùng rồi thay vùng nhớ cũ.
    void resize(int new_capacity) {
        // Cấp phát vùng lưu trữ mới với sức chứa được yêu cầu.
        HoSoTruyXuat* new_data = new HoSoTruyXuat[new_capacity];
        // Chỉ sao chép các phần tử hợp lệ, không sao chép các ô còn trống.
        for (int i = 0; i < m_size; ++i) {
            new_data[i] = data[i];
        }
        // Giải phóng vùng nhớ cũ sau khi các phần tử đã được sao chép.
        delete[] data;
        // Chuyển mảng sang vùng nhớ mới và cập nhật sức chứa.
        data = new_data;
        m_capacity = new_capacity;
    }

public:
    // Khởi tạo mảng rỗng với sức chứa ban đầu là một phần tử.
    MangDongBenhNhan() : m_size(0), m_capacity(1) {
        // Cấp phát vùng nhớ ban đầu theo sức chứa đã thiết lập.
        data = new HoSoTruyXuat[m_capacity];
    }

    // Cấm sao chép nông để tránh hai đối tượng cùng giải phóng một vùng nhớ.
    MangDongBenhNhan(const MangDongBenhNhan&) = delete;
    // Cấm phép gán sao chép vì lớp trực tiếp quản lý vùng nhớ động.
    MangDongBenhNhan& operator=(const MangDongBenhNhan&) = delete;

    // Giải phóng vùng nhớ do đối tượng đang sở hữu.
    ~MangDongBenhNhan() {
        delete[] data;
    }

    // Thêm phần tử vào cuối mảng; sức chứa được nhân đôi khi mảng đã đầy.
    void push_back(const HoSoTruyXuat& value) {
        // Mở rộng vùng nhớ trước khi thêm nếu không còn chỗ trống.
        if (m_size == m_capacity) {
            resize(m_capacity * 2);
        }
        // Ghi phần tử mới vào ô kế tiếp sau các phần tử hiện có.
        data[m_size] = value;
        // Tăng số lượng phần tử sau khi thêm thành công.
        m_size++;
    }

    // Xóa phần tử tại index bằng cách dồn các phần tử phía sau sang trái.
    void erase(int index) {
        // Ném ngoại lệ nếu chỉ số nằm ngoài vùng phần tử đang được sử dụng.
        if (index < 0 || index >= m_size) {
            throw std::out_of_range("Index out of bounds");
        }
        // Dời các phần tử phía sau một ô để lấp khoảng trống tại vị trí bị xóa.
        for (int i = index; i < m_size - 1; ++i) {
            data[i] = data[i + 1];
        }
        // Giảm kích thước; sức chứa đã cấp phát được giữ nguyên.
        m_size--;
    }

    // Đặt số phần tử về 0 nhưng giữ lại vùng nhớ để tái sử dụng.
    void clear() {
        m_size = 0;
    }

    // Trả về số phần tử đang có.
    int size() const { return m_size; }
    // Cho biết mảng có đang rỗng hay không.
    bool empty() const { return m_size == 0; }

    // Truy cập phần tử có thể chỉnh sửa; bên gọi phải truyền chỉ số hợp lệ.
    HoSoTruyXuat& operator[](int index) { return data[index]; }
    // Truy cập chỉ đọc phần tử; bên gọi phải truyền chỉ số hợp lệ.
    const HoSoTruyXuat& operator[](int index) const { return data[index]; }
};