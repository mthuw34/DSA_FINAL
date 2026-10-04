# Đang khám

Luồng dữ liệu: SQLite → vector lịch phân bác sĩ/lịch sử khám → ExamCore → hiển thị hoặc lưu thay đổi.

- **Presentation:** `DangKhamManager.cpp`, `NhapChanDoan.cpp` và `main.cpp` nhận input, gọi tầng xử lý và hiển thị kết quả.
- **DSA Core:** `ExamCore.h` chứa bảng băm tự cài đặt để đối chiếu check-in, kiểm tra thời gian/trạng thái bằng C++, tìm ca trên vector và Merge Sort ổn định tự cài đặt cho danh sách đang khám. Các hàm này không phụ thuộc SQLite.
- **Persistence:** `DatabaseDangKham.cpp` nạp toàn bộ `ket_qua_kham` và `dang_kham` bằng SELECT thuần; tạo/bổ sung schema; ghi các bản ghi mà tầng xử lý đã chọn. Không lọc hoặc sắp xếp danh sách bằng SQL.

Ca mới chỉ được nhận khi đã đến giờ bắt đầu và chưa hết giờ dự kiến. Ca đã nhận vẫn đang khám cho đến khi bác sĩ kết thúc thực tế; đồng bộ không mở lại ca đã kết thúc và không ghi đè chẩn đoán, đơn thuốc, lời nhắc hay giờ check-in đã lưu. Thời gian nguồn dùng định dạng địa phương YYYY-MM-DD HH:MM:SS (chấp nhận T thay khoảng trắng). Nguồn chưa có giờ check-in gốc nên giờ bắt đầu là giá trị thay thế khi thiếu.

Đối chiếu lịch sử bằng bảng băm có chi phí trung bình O(n + m), trường hợp xấu O(nm); sắp ca đang khám O(k log k) thời gian và O(k) bộ nhớ phụ; tìm một ca O(n). Các vector được nạp lại khi thao tác để nhận các ca mới từ chương trình phân bác sĩ.

`WHERE` chỉ còn trong UPDATE để lưu đúng ID và bảo vệ ca nếu phiên khác vừa kết thúc. Đồng bộ ghi trong một giao dịch; lỗi ghi sẽ rollback. Database cũ có `checkin_time NOT NULL` được giữ tương thích, các cột còn thiếu được bổ sung.

Kiểm thử từ root repo: `python DANG_KHAM/tests/regression.py` (cần Python, g++ và SQLite). Bản kiểm thử chặn câu đọc SQL chứa WHERE, ORDER BY, JOIN, GROUP BY hoặc LIMIT.
