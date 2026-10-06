# Phân tích luồng dữ liệu chức năng truy xuất bệnh nhân

## 1. Phạm vi

Tài liệu mô tả luồng truy xuất và sắp xếp bệnh nhân trong chức năng
`TRUY_XUAT_BENH_NHAN`, cùng với bước khởi tạo các bảng hàng đợi được triển khai
trong `src/TAO_BANG/DBTaoBang.cpp`.

## 2. Các thành phần tham gia

| Thành phần | Vai trò |
|---|---|
| `src/main.cpp` | Hiển thị menu, nhận lựa chọn và gọi chức năng tương ứng. |
| `src/TRUY_XUAT/TruyXuat.h` | Khai báo hồ sơ bệnh nhân, cấu hình khoa và các lớp/hàm liên quan. |
| `src/TRUY_XUAT/QuanLyHangDoi.cpp` | Điều phối việc đọc dữ liệu, tạo bộ đệm, sắp xếp và ghi kết quả. |
| `src/TRUY_XUAT/DBTruyXuat.cpp` | Đọc dữ liệu từ `priority.db`; xóa dữ liệu hàng đợi cũ và ghi kết quả vào `truyXuat.db`. |
| `src/TRUY_XUAT/ThuatToanSapXep.cpp` | So sánh mức ưu tiên và sắp xếp mảng bằng Merge Sort. |
| `src/TRUY_XUAT/MangDongBenhNhan.h` | Cung cấp mảng động tự cài đặt để lưu các hồ sơ trong bộ nhớ. |
| `src/XOA_BENH_NHAN/XoaBenhNhan.cpp` | Chuyển yêu cầu xóa từ menu tới tầng xử lý dữ liệu. |
| `src/XOA_BENH_NHAN/DBXoaBenhNhan.cpp` | Tìm/xóa hồ sơ trong mảng động rồi cập nhật CSDL nguồn và hàng đợi. |
| `src/XOA_BENH_NHAN/XoaBenhNhan.h` | Khai báo giao diện xóa bệnh nhân và lớp xử lý dữ liệu. |
| `src/TAO_BANG/DBTaoBang.cpp` | Tạo một bảng hàng đợi trong `truyXuat.db` cho từng khoa cấu hình. |
| `src/TAO_BANG/TaoBangTruyXuat.cpp` | Gọi hàm tạo bảng và chuyển kết quả thành mã thoát của tiến trình. |

## 3. Sơ đồ luồng tổng quát

```text
Người dùng
   │ chọn chức năng trong menu
   ▼
main()
   ├── chọn 1: truy xuất và sắp xếp
   │      └── QuanLyHangDoi::taiVaXuLyBenhNhan()
   │             ├── DBTruyXuat::docDanhSachBenhNhan(records)
   │             │      └── đọc priority.db / priority_checkins
   │             ├── tạo buffer từ records
   │             ├── ThuatToanSapXep::sapXepTron(records, buffer, ...)
   │             │      ├── chia đoạn đệ quy
   │             │      ├── tron(...)
   │             │      └── xetUuTien(...)
   │             └── DBTruyXuat::ghiDanhSachDaSapXep(records)
   │                    └── ghi truyXuat.db / bảng riêng theo khoa
   ├── chọn 2: xóa bệnh nhân theo ID
   │      └── XoaBenhNhan::xoaBenhNhan(id)
   │             └── DBXoaBenhNhan::xoaBenhNhan(id)
   │                    ├── đọc danh sách từ priority.db
   │                    ├── tìm và xóa lượt khám trong mảng động
   │                    ├── sắp xếp lại danh sách còn lại
   │                    ├── ghi lại priority.db
   │                    └── ghi lại các bảng trong truyXuat.db
   └── chọn 3: xóa toàn bộ (sau khi xác nhận)
          └── XoaBenhNhan::xoaTatCaBenhNhan()
                 └── DBXoaBenhNhan::xoaTatCaBenhNhan()
                        ├── ghi danh sách rỗng vào priority.db
                        └── ghi danh sách rỗng vào truyXuat.db
```

Các bảng đích cần được tạo trước bằng tiến trình khởi tạo bảng:

```text
TaoBangTruyXuat.cpp::main()
   └── DBTaoBang::taoBangTruyXuat()
          ├── mở hoặc tạo truyXuat.db
          ├── duyệt CauHinhTruyXuat::danhSachKhoa
          ├── CREATE TABLE IF NOT EXISTS cho từng khoa
          └── COMMIT nếu toàn bộ thành công, nếu không thì ROLLBACK
```

## 4. Luồng truy xuất và xử lý dữ liệu

### Bước 1: Nhận yêu cầu từ người dùng

`main()` trong `src/main.cpp` tạo một đối tượng `QuanLyHangDoi`, sau đó liên tục
hiển thị menu. Khi người dùng chọn `1`, chương trình gọi
`QuanLyHangDoi::taiVaXuLyBenhNhan()`. Nếu hàm trả về `true`, menu thông báo thành
công; nếu trả về `false`, menu in thông báo lỗi.

### Bước 2: Đọc dữ liệu nguồn từ cơ sở dữ liệu

`taiVaXuLyBenhNhan()` tạo `MangDongBenhNhan records` và truyền nó vào
`DBTruyXuat::docDanhSachBenhNhan(records)`.

Hàm đọc dữ liệu thực hiện các thao tác sau:

1. Xóa nội dung hiện có của `records`.
2. Mở `THAY_DOI_MUC_DO_UU_TIEN/db/priority.db` ở chế độ chỉ đọc.
3. Truy vấn các cột `checkin_id`, `patient_id`, `department`, `checkin_time`,
   `base_priority`, `current_priority`, `last_update` từ bảng
   `priority_checkins`.
4. Với mỗi dòng SQLite trả về, tạo một `HoSoTruyXuat` và chép các cột tương ứng
   vào hồ sơ.
5. Dùng `CauHinhTruyXuat::thuTuKhoa()` để chuyển tên khoa thành thứ tự khoa.
   Khoa không có trong cấu hình nhận giá trị `99`.
6. Thêm hồ sơ vào `records` bằng `push_back()`.
7. Giải phóng câu truy vấn và đóng kết nối SQLite.

Nếu mở hoặc truy vấn cơ sở dữ liệu thất bại, hàm trả về `false`; lớp quản lý
dừng luồng và không thực hiện bước sắp xếp/ghi.

### Bước 3: Lưu hồ sơ trong mảng động

`MangDongBenhNhan` lưu các `HoSoTruyXuat` trong vùng nhớ được cấp phát động.
Khi mảng đầy, `push_back()` gọi `resize()` để tăng sức chứa; nhờ vậy số hồ sơ
đọc vào không bị giới hạn bởi một kích thước mảng cố định.

Sau khi đọc thành công, `taiVaXuLyBenhNhan()` tạo `buffer` và sao chép từng phần
tử từ `records` vào đó. Bộ đệm này được Merge Sort sử dụng để ghép các đoạn đã
sắp xếp.

### Bước 4: Sắp xếp hồ sơ

Nếu danh sách không rỗng, lớp quản lý gọi
`ThuatToanSapXep::sapXepTron(records, buffer, 0, records.size() - 1)`.
`sapXepTron()` chia đoạn hiện tại làm hai nửa cho đến khi mỗi đoạn chỉ còn tối
đa một phần tử, rồi gọi `tron()` để ghép hai nửa theo thứ tự.

`tron()` gọi `xetUuTien()` để xác định hồ sơ nào đứng trước. Các khóa so sánh
được xét lần lượt:

1. Thứ tự khoa (`departmentOrder`) tăng dần.
2. Mức ưu tiên hiện tại (`currentPriority`) tăng dần; do đó giá trị số nhỏ hơn
   được xếp trước.
3. Trạng thái ca trở nặng lâm sàng (`isTroNangLamSang`) được ưu tiên nếu khác
   nhau.
4. Mức ưu tiên ban đầu (`basePriority`) tăng dần.
5. Thời điểm vào khám (`checkinTime`) tăng dần.
6. Thời điểm cập nhật gần nhất (`lastUpdate`) tăng dần.
7. Mã lượt khám (`checkinId`) tăng dần để phá hòa xác định.

Khi hai hồ sơ tương đương theo tiêu chí so sánh, `tron()` chọn phần tử ở nửa
trái trước, nhờ đó giữ thứ tự tương đối của các phần tử tương đương trong quá
trình ghép.

**Lưu ý theo mã hiện tại:** `docDanhSachBenhNhan()` không nạp cột thể hiện
`isTroNangLamSang`; trường này được khởi tạo mặc định là `false` trong
`HoSoTruyXuat`. Vì vậy tiêu chí ca trở nặng chỉ có tác dụng nếu trường được gán
giá trị ở một nơi khác trước khi so sánh; trong luồng đọc hiện tại, tất cả hồ sơ
đều có giá trị mặc định.

### Bước 5: Ghi kết quả đã sắp xếp

Sau khi sắp xếp xong, `taiVaXuLyBenhNhan()` gọi
`DBTruyXuat::ghiDanhSachDaSapXep(records)`. Hàm này:

1. Mở `TRUY_XUAT_BENH_NHAN/db/truyXuat.db` ở chế độ đọc/ghi.
2. Bắt đầu một giao dịch SQLite.
3. Xóa dữ liệu hiện tại khỏi các bảng khoa để thay thế bằng danh sách mới.
4. Chuẩn bị câu lệnh `INSERT` cho từng bảng khoa.
5. Duyệt các hồ sơ đã sắp xếp; dùng `departmentOrder` để xác định bảng nhận
   hồ sơ và đánh số `retrieval_order` riêng trong từng khoa.
6. Ghi các trường lượt khám, bệnh nhân, khoa, thời gian, ưu tiên và thời điểm
   cập nhật. Nếu `lastUpdate` rỗng, cột tương ứng được ghi là `NULL`.
7. Giải phóng các câu lệnh đã chuẩn bị.
8. Nếu mọi thao tác thành công, xác nhận bằng `COMMIT`; nếu không, hoàn tác bằng
   `ROLLBACK`.

Hồ sơ có `departmentOrder` nhỏ hơn `1` hoặc lớn hơn số khoa đã cấu hình bị bỏ
qua trong bước ghi. Điều này bao gồm khoa không được ánh xạ (giá trị `99`).

### Bước 6: Xóa một bệnh nhân

Khi người dùng chọn `2`, `main()` đọc `patientId` rồi gọi
`XoaBenhNhan::xoaBenhNhan(patientId)`. Hàm giao diện này chuyển yêu cầu tới
`DBXoaBenhNhan::xoaBenhNhan(patientId)`.

Hàm xử lý dữ liệu thực hiện các bước sau:

1. Từ chối mã bệnh nhân nhỏ hơn hoặc bằng `0`.
2. Đọc các lượt khám từ `priority.db` vào `MangDongBenhNhan records`.
3. Duyệt mảng tuần tự, tìm lượt đầu tiên có `patientId` trùng với mã cần xóa.
4. Xóa phần tử bằng `erase()`; các phần tử phía sau được dồn sang trái.
5. Nếu không tìm thấy, báo không tìm thấy và không ghi lại cơ sở dữ liệu.
6. Tạo bộ đệm, sắp xếp lại danh sách còn lại bằng Merge Sort.
7. Ghi danh sách mới vào `priority.db` trong một giao dịch.
8. Nếu bước 7 thành công, cập nhật các bảng hàng đợi trong `truyXuat.db`.

**Lưu ý:** vòng tìm kiếm dừng ngay sau lượt khám đầu tiên khớp `patientId`. Nếu
một bệnh nhân có nhiều lượt khám trong `priority_checkins`, luồng hiện tại chỉ
xóa lượt đầu tiên tìm được. Hai thao tác cập nhật `priority.db` và `truyXuat.db`
được thực hiện tuần tự, không nằm trong cùng một giao dịch liên cơ sở dữ liệu;
nếu bước ghi hàng đợi thất bại sau khi nguồn đã cập nhật, hai CSDL có thể tạm
thời không đồng nhất và chương trình sẽ in thông báo lỗi cụ thể.

### Bước 7: Xóa toàn bộ bệnh nhân

Khi người dùng chọn `3`, `main()` yêu cầu nhập `Y` hoặc `y` để xác nhận. Khi đã
xác nhận, yêu cầu được chuyển qua `XoaBenhNhan::xoaTatCaBenhNhan()` tới
`DBXoaBenhNhan::xoaTatCaBenhNhan()`. Hàm tạo một `MangDongBenhNhan` rỗng, ghi
danh sách rỗng vào `priority.db`, rồi ghi danh sách rỗng vào các bảng hàng đợi
của `truyXuat.db`. Mỗi lần ghi có giao dịch riêng; hàm chỉ trả về thành công
nếu cả hai lần ghi đều thành công.

## 8. Luồng khởi tạo bảng đích

`src/TAO_BANG/TaoBangTruyXuat.cpp` gọi
`DBTaoBang::taoBangTruyXuat()` và trả mã thoát `0` khi thành công, `1` khi thất
bại.

`taoBangTruyXuat()` mở hoặc tạo tệp
`TRUY_XUAT_BENH_NHAN/db/truyXuat.db`, bắt đầu giao dịch, rồi lặp qua
`CauHinhTruyXuat::danhSachKhoa`. Với mỗi khoa, hàm lấy `tenBang` để tạo bảng nếu
bảng chưa tồn tại. Mỗi bảng có các cột:

| Cột | Ý nghĩa |
|---|---|
| `retrieval_order` | Vị trí của bệnh nhân trong hàng đợi của khoa, đồng thời là khóa chính. |
| `checkin_id` | Mã lượt khám, bắt buộc và duy nhất trong bảng. |
| `patient_id` | Mã bệnh nhân. |
| `department` | Tên khoa. |
| `checkin_time` | Thời điểm đăng ký khám. |
| `base_priority` | Mức ưu tiên gốc. |
| `current_priority` | Mức ưu tiên hiện tại. |
| `last_update` | Thời điểm cập nhật gần nhất, có thể là `NULL`. |

Nếu tạo đủ bảng và xác nhận giao dịch thành công, kết nối được đóng và hàm trả
về `true`. Nếu tạo bảng hoặc xác nhận giao dịch thất bại, hàm thử `ROLLBACK`,
đóng kết nối rồi trả về `false`. `IF NOT EXISTS` bảo đảm chạy lại tiến trình
khởi tạo không ghi đè các bảng đã có.

## 9. Dữ liệu vào, dữ liệu trung gian và dữ liệu ra

| Giai đoạn | Nguồn/đích | Dữ liệu |
|---|---|---|
| Đầu vào | `priority.db`, bảng `priority_checkins` | Mã lượt khám, mã bệnh nhân, khoa, thời gian khám, ưu tiên gốc/hiện tại, lần cập nhật gần nhất. |
| Trung gian | `MangDongBenhNhan records` | Các hồ sơ sau khi đọc, rồi được sắp xếp trực tiếp trong mảng. |
| Bộ đệm | `MangDongBenhNhan buffer` | Bản sao ban đầu của danh sách, dùng khi ghép các đoạn trong Merge Sort. |
| Đầu ra | `truyXuat.db`, các bảng `queue_<ten_khoa>` trong cấu hình | Hồ sơ được nhóm theo khoa và đánh số thứ tự hàng đợi sau khi sắp xếp. |

## 10. Xử lý lỗi và tính toàn vẹn dữ liệu

- Không mở được cơ sở dữ liệu nguồn: dừng trước khi sắp xếp và ghi kết quả.
- Lỗi khi đọc SQLite: xóa danh sách đọc dở, đóng tài nguyên và báo thất bại.
- Không mở được cơ sở dữ liệu đích: không bắt đầu thao tác ghi.
- Lỗi khi xóa hoặc chèn hàng đợi: giao dịch ghi bị hoàn tác để tránh để lại
  trạng thái cập nhật một phần.
- Lỗi khi tạo bảng: giao dịch khởi tạo bị hoàn tác; hàm báo thất bại.
- Các câu lệnh SQLite đã chuẩn bị được giải phóng và kết nối được đóng sau mỗi
  lần thao tác.

> Các đường dẫn cơ sở dữ liệu trong mã là đường dẫn tương đối. Hãy chạy chương
> trình từ thư mục gốc phù hợp của dự án để SQLite mở đúng tệp.

## CÁC THUẬT TOÁN CHÍNH TRONG CHƯƠNG TRÌNH

### 1. Thuật toán Sắp xếp: Merge Sort (Sắp xếp trộn)
Bạn đã sử dụng thuật toán Merge Sort làm "bộ não" (DSA Core) để sắp xếp danh sách bệnh nhân.

	Hàm liên quan: ThuatToanSapXep::sapXepTron() và ThuatToanSapXep::tron().

	Logic hoạt động (Divide-and-Conquer): Thuật toán chia danh sách bệnh nhân ra làm hai nửa liên tục cho đến khi mỗi phần nhỏ chỉ còn 1 bệnh nhân. Sau đó, hàm tron() sẽ gộp hai nửa đã sắp xếp lại với nhau bằng cách dùng hai con trỏ chạy từ trái qua phải, so sánh và đưa phần tử phù hợp vào mảng phụ trợ (buffer). 

	Tại sao chọn Merge Sort? Merge Sort luôn đảm bảo thời gian chạy là Θ(n log⁡n ) trong mọi trường hợp (best, average, worst case). Điều này cực kỳ quan trọng cho một hệ thống y tế khẩn cấp vì nó cam kết thời gian phản hồi ổn định (guaranteed schedule), không bị chậm đi ngay cả khi dữ liệu quá lớn hoặc đầu vào xấu đi. 

	Sự đánh đổi (Space-Time Tradeoff): Merge Sort không phải là thuật toán tại chỗ (in-place). Bạn đã phải sử dụng mảng buffer với chi phí bộ nhớ là O(n) (auxiliary space) để chứa dữ liệu tạm thời khi gộp. Bù lại, điều này giúp hệ thống đạt tốc độ Θ(n log⁡n ) và bảo toàn tính ổn định (stability). 

### 2. Thuật toán Tìm kiếm: Linear Search (Tìm kiếm tuyến tính)
Khi bạn thực hiện chức năng xóa một bệnh nhân cụ thể trên mảng.

	Hàm liên quan: Hàm tìm kiếm ẩn trong vòng lặp for của DBXoaBenhNhan::xoaBenhNhan(int patientId).

	Logic hoạt động: Thuật toán duyệt tuần tự (linear scan) từng hồ sơ bệnh nhân từ đầu đến cuối danh sách (mảng records) để so sánh patientId truyền vào với ID của từng hồ sơ.

	Vì sao dùng Linear Search? Vì mảng động MangDongBenhNhan đang được sắp xếp ưu tiên theo 7 tiêu chí y khoa khác nhau, không được sắp xếp tăng dần theo patientId. Do dữ liệu ID không có thứ tự, việc quét tuần tự với chi phí O(n) là bắt buộc. 

### 3. Cấu trúc Dữ liệu: Mảng động (Dynamic Array)
Thay vì dùng thư viện có sẵn std::vector, bạn đã tự cài đặt class MangDongBenhNhan.

	Hàm liên quan: push_back(const HoSoTruyXuat& value), resize(int new_capacity), erase(int index).

	Logic của thao tác Thêm (push_back): Khi mảng đầy (m_size == m_capacity), hệ thống tự động gọi hàm resize để cấp phát một mảng mới có kích thước gấp đôi (doubling) và copy dữ liệu cũ sang. Mặc dù thao tác copy này tốn thời gian O(n), nhưng nó rất hiếm khi xảy ra. Tính trung bình, chi phí thêm mới mỗi bệnh nhân đạt mức cực nhanh: O(1) khấu hao (amortized cost).

	Logic của thao tác Xóa (erase): Khi xóa một bệnh nhân ở vị trí index, các phần tử đứng phía sau (những người sống sót - survivors) sẽ tự động dịch chuyển sang trái một bước để lấp đầy khoảng trống. Thao tác dịch chuyển này tiêu tốn thời gian O(n). Cấu trúc mảng phải làm điều này để đảm bảo dữ liệu luôn liên tục trong bộ nhớ, không có lỗ hổng. 

### 4. Cơ chế Xử lý Tie-breaker (Giải quyết xung đột)
Trong hàm so sánh xetUuTien của bạn, có một cơ chế logic cực kỳ quan trọng là Multi-key Sorting (Sắp xếp đa khóa). 

	Logic hoạt động: Hàm này sẽ xét từ thứ tự Khoa → Độ khẩn cấp hiện tại → Khẩn cấp đột xuất → Khẩn cấp ban đầu → Giờ thay đổi tình trạng → Giờ bốc số. 

	Chốt chặn cuối cùng (Tie-breaker): Cuối cùng, hàm dùng left.checkinId < right.checkinId để so sánh. Đây là ID bốc số tăng dần đều. Nếu hai người giống nhau y đúc ở mọi tiêu chí thời gian và sức khỏe, ID nào nhỏ hơn (vào hệ thống trước) sẽ ưu tiên hơn. Kỹ thuật này giúp thuật toán chia-để-trị của bạn hoạt động ổn định tuyệt đối, không vô tình xáo trộn trật tự ngẫu nhiên. 

