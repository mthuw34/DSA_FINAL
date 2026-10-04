# Quản lý bệnh nhân

Luồng dữ liệu: SQLite → bản ghi trong vector → DSA Core → hiển thị hoặc lưu thay đổi.

- **Presentation:** menu, nhập CSV/thông tin và xuất phiếu trong các chương trình con.
- **DSA Core:** `src/PatientCore.h` chứa bảng băm tự cài đặt để tìm ID, nhận diện bệnh nhân trùng khi import; tìm check-in trên vector; Merge Sort ổn định tự cài đặt để sắp theo khoa, ưu tiên, giờ đến và ID. Quy định giờ nhận bệnh nhân nằm trong `CHECK_IN/khoa.cpp`: 07:30–10:00 và 13:00–15:00; Khoa Cấp cứu giữ 24/7.
- **Persistence:** `src/HospitalPersistence.h` nạp toàn bộ bảng patients/checkins bằng SELECT thuần. Không dùng SQL để tìm, lọc, nối bảng hoặc sắp thứ tự. INSERT/UPDATE/DELETE trong chương trình con chỉ lưu thao tác đã được xác định trong bộ nhớ.

Tìm ID bằng bảng băm có chi phí trung bình O(1) sau bước xây dựng O(n), trường hợp xấu O(n). Sắp xếp check-in dùng O(n log n) thời gian và O(n) bộ nhớ phụ. Tìm check-in bằng duyệt vector dùng O(n). Import dùng khóa ghép có độ dài từng trường, giữ phân biệt NULL với chuỗi rỗng.

`WHERE` chỉ còn trong lệnh UPDATE/DELETE để ghi đúng bản ghi. Unique index của patient_id chỉ là ràng buộc toàn vẹn lúc lưu; không được dùng thay thuật toán tìm kiếm hay kiểm tra trùng. Check-in/import nạp lại dữ liệu trong giao dịch ghi ngắn để chống hai phiên cùng nhận trùng. Không xóa bộ đếm check-in khi xóa dữ liệu.

Kiểm thử từ root repo: `python QUAN_LY_BENH_NHAN/tests/regression.py` (cần Python, g++ và SQLite). Bản kiểm thử chặn các câu đọc SQL chứa WHERE, ORDER BY, JOIN, GROUP BY hoặc LIMIT.
