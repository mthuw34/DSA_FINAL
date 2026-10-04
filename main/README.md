# Web quản lý khám bệnh

`main.cpp` đăng ký HTTP routes bằng Crow. `WebService.h/.cpp` chứa các hàm nhận dữ liệu,
trả JSON và gọi nghiệp vụ hiện có, không đọc `cin`. SQLite chỉ nạp dữ liệu;
việc tìm bệnh nhân, ghép tên, kiểm tra trùng và sắp thứ tự được làm bằng C++.

## Build và chạy trên Windows

Cần g++ MinGW và SQLite (máy hiện tại có trong `C:/msys64/ucrt64`). Crow, Asio và
nlohmann/json đã có trong `include/`. Chạy từ thư mục gốc dự án:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File main/build.ps1
.\main\build\hospital_web.exe
```

Mở `http://127.0.0.1:18080`. Giao diện tiếng Việt gồm Tổng quan, Bệnh nhân,
Hàng đợi, Lịch khám, Đang khám và Bác sĩ. Có tìm kiếm, lọc khoa, phân trang,
thêm/sửa/xóa hồ sơ, check-in, điều chỉnh ưu tiên, phân bác sĩ, chẩn đoán và kết thúc khám.
HTML/CSS/JavaScript nằm trong `main/web/`, được Crow phục vụ cùng địa chỉ với API.
Không cần cài Node/npm để chạy giao diện. Có thể chọn thư mục dữ liệu và port khác:

```powershell
.\main\build\hospital_web.exe "D:\GitHub\DSA_FINAL" 18081
```

Nếu server cũ đang chạy, bấm Ctrl+C trong terminal đó, build lại rồi chạy lại server.
Sau check-in, bệnh nhân tự xuất hiện ở **Hàng đợi** và rời danh sách **Hồ sơ chờ check-in**.
Hồ sơ gốc vẫn được lưu để tra cứu trong lượt khám. Server tự đồng bộ mỗi 5 giây,
kể cả khi đóng trình duyệt; giao diện tự tải dữ liệu mỗi 5 giây khi không mở biểu mẫu.
Tải nền không làm mờ/khóa giao diện, không dựng lại bảng hoặc menu nếu nội dung không đổi.
Trong **Lịch khám → Phân bác sĩ**, chọn **Phân một khoa** hoặc **Phân theo bác sĩ**.
Ca tới giờ được tự nhận khi bác sĩ trống; bác sĩ chỉ khám một ca cùng lúc.
Trong **Bác sĩ**, trạng thái gồm Đang khám, Đang trong ca trực, Ngoài ca/nghỉ và Bận đột xuất.
**Xem ca trực** hiển thị 7 ngày từ ngày hiện tại của server, kèm giờ bắt đầu/kết thúc và ngày nghỉ.
Khoa thường làm Thứ Hai–Thứ Sáu, 07:00–11:30 và 13:00–17:00;
cấp cứu trực 24 giờ (00:00 đến 00:00 hôm sau), nghỉ 24 giờ theo pha lịch web.
Lịch hiển thị dùng cùng quy tắc với bộ phân bác sĩ. Trực/nghỉ thủ công và bận đột xuất
được ghi riêng trong hộp xem lịch để phân biệt với lịch tự động.
**Ca trực** cho chọn tự động theo lịch, đang trực thủ công hoặc nghỉ ca.
**Bận đột xuất** nhận số phút (1–1440) và lý do; tự hết khi tới hạn hoặc bấm **Hết bận**.
Báo bận/nghỉ trả các lịch chưa bắt đầu về hàng đợi để phân lại, giữ ca đang khám.
Trạng thái được lưu trong SQLite và giữ sau khi khởi động lại server.
Trong Đang khám, chọn **Đã hoàn tất** để xem lịch sử và thông tin điều trị đã lưu.

Server tự tạo thư mục/bảng còn thiếu và bổ sung height/weight/bmi cho database cũ.
Không xóa dữ liệu đang có. Các request nghiệp vụ được xử lý lần lượt bằng mutex.
Server mặc định nghe trên localhost. Cấu hình `HOSPITAL_PASSWORD` bật đăng nhập
HTTP Basic bằng tên `admin`; chưa có phân quyền theo từng nhân viên.
Xem [hướng dẫn đưa lên Internet](deploy/README.md) và `Dockerfile` ở gốc repo.

## API

Thành công: `{"ok":true,"data":...}`. Lỗi: `{"ok":false,"error":"..."}`.
Body ghi dữ liệu phải là JSON object, `Content-Type: application/json`.
Sai dữ liệu trả 400, không tìm thấy trả 404, xung đột nghiệp vụ trả 409, lỗi server trả 500.

| Method | URL | Chức năng |
|---|---|---|
| GET | `/api/health` | Trạng thái server |
| GET | `/api/departments` | Khoa và trạng thái nhận check-in |
| GET | `/api/patients?q=...` | Danh sách/tìm tên, điện thoại hoặc ID; chuỗi có phân biệt hoa thường |
| POST | `/api/patients` | Tạo bệnh nhân |
| GET | `/api/patients/{id}` | Lấy bệnh nhân |
| PATCH | `/api/patients/{id}` | Sửa các trường được gửi |
| DELETE | `/api/patients/{id}` | Xóa bệnh nhân chưa check-in |
| GET | `/api/checkins` | Phiếu check-in sắp theo khoa, ưu tiên, giờ |
| POST | `/api/checkins` | Check-in bệnh nhân |
| POST | `/api/queue/sync` | Đồng bộ hospital → priority → queue và các ca đến giờ khám |
| GET | `/api/queue` | Hàng chờ theo ưu tiên hiện tại; loại ca đã phân bác sĩ |
| PATCH | `/api/checkins/{id}/priority` | Đổi ưu tiên của ca chưa phân bác sĩ |
| GET | `/api/doctors` | Bác sĩ từ CSV |
| PATCH | `/api/doctors/{id}/status` | Cập nhật ca trực hoặc bận đột xuất |
| GET | `/api/assignments` | Lịch phân bác sĩ đã lưu |
| POST | `/api/assignments` | Phân bác sĩ một khoa hoặc tất cả |
| POST | `/api/exams/sync` | Như queue/sync; nhận các ca đã tới giờ khám |
| GET | `/api/exams` | Ca đang khám |
| GET | `/api/exams?active=false` | Toàn bộ lịch sử ca khám |
| PATCH | `/api/exams/{checkin_id}/diagnosis` | Lưu chẩn đoán, đơn thuốc, lời nhắc |
| POST | `/api/exams/{checkin_id}/finish` | Ghi giờ kết thúc thực tế |

Tạo bệnh nhân cần `name`, `birth_date` (YYYY-MM-DD). Các trường tùy chọn:
`phone`, `gender`, `hometown`, `address`, `height` (cm), `weight` (kg).
Server tính tuổi và BMI, không nhận tuổi/BMI từ client.

```javascript
const call = async (url, method = 'GET', data) => {
  const response = await fetch(url, {
    method,
    headers: { 'Content-Type': 'application/json' },
    ...(data === undefined ? {} : { body: JSON.stringify(data) })
  });
  const result = await response.json();
  if (!response.ok) throw new Error(result.error);
  return result.data;
};

const patient = await call('/api/patients', 'POST', {
  name: 'Nguyễn Văn A', birth_date: '2000-01-15', height: 170, weight: 65
});
const ticket = await call('/api/checkins', 'POST', {
  patient_id: patient.id, department: 'Khoa Cap cuu', priority: 1
});
await call('/api/queue/sync', 'POST');
await call(`/api/checkins/${ticket.checkin_id}/priority`, 'PATCH', { priority: 2 });
await call('/api/assignments', 'POST', { department: 'Khoa Cap cuu' });
await call('/api/exams/sync', 'POST');
const active = await call('/api/exams');
// Chỉ lưu cho ca có mặt trong danh sách đang khám.
if (active.some(exam => exam.checkin_id === ticket.checkin_id)) {
  await call(`/api/exams/${ticket.checkin_id}/diagnosis`, 'PATCH', {
    diagnosis: 'Chẩn đoán mẫu', prescription: 'Đơn thuốc mẫu', reminder: 'Tái khám'
  });
  await call(`/api/exams/${ticket.checkin_id}/finish`, 'POST');
}
```

GET không ghi dữ liệu. Check-in tự đồng bộ hàng đợi; tác vụ nền thử lại nếu tạm lỗi.
PATCH ưu tiên cập nhật priority.db; POST assignments luôn đồng bộ queue trước khi phân.
POST assignments nhận `{department: "Khoa Noi"}` để phân một khoa,
`{doctor_id: "BS001"}` để phân cho một bác sĩ (tự lấy khoa của bác sĩ), hoặc `{}` để phân tất cả khoa.
PATCH trạng thái nhận `{duty_mode: "auto" | "on_duty" | "off_duty"}` và/hoặc
`{busy_minutes: 30, busy_reason: "Họp gấp"}`. Gửi `{busy_minutes: 0}` để hết bận sớm.
Bác sĩ bận/nghỉ/đang khám không nhận lịch mới. Bác sĩ tự động ngoài giờ có thể nhận lịch trong ca tiếp theo.
Gọi lại không phân lại ca đã lưu;
lịch bận bác sĩ được khôi phục từ các ca trước để tránh chồng lịch.
Thuật toán web mô phỏng thời lượng 10–30 phút; bận đột xuất do người dùng khai báo.
Pha trực cấp cứu web ổn định theo thứ tự CSV, không đổi ngẫu nhiên khi phân lại.

Check-in thường tuân theo giờ của `khoa.cpp`; cấp cứu nhận 24/7.
Ngoài giờ, ưu tiên 1–2 có thể gửi `transfer_to_emergency: true` để chuyển cấp cứu.
Mỗi bệnh nhân vẫn chỉ có một check-in theo schema hiện tại. Ca được nhận vào đang khám
chỉ khi giờ bắt đầu đã tới và bác sĩ chưa khám ca khác, kể cả server khởi động trễ; ca đã nhận chỉ kết thúc
khi gọi API finish. Giờ dùng timezone của máy chạy server, nên cấu hình máy theo giờ Việt Nam.
Tăng ưu tiên tự động theo thời gian chưa được chạy nền bởi server này.
Không chạy chương trình CLI phân bác sĩ song song với server vì CLI cũ không khôi phục lịch đã lưu.

## Kiểm tra

```powershell
python main/tests/api_smoke.py
node main/tests/ui_smoke.cjs
```

Kiểm thử khởi động server trên port ngẫu nhiên với các database mới trong thư mục tạm
dưới `main/build/`, không ghi vào database của dự án.
Kiểm thử UI dùng DOM giả để kiểm tra các màn hình, tìm kiếm, biểu mẫu, dữ liệu gửi API,
thoát HTML và thông báo khi đã lưu nhưng tải lại bị lỗi; không thay thế kiểm tra bố cục trên trình duyệt.
