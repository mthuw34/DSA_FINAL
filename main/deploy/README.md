# Đưa MediFlow lên Internet

Ứng dụng C++ cần web service chạy server. Dockerfile đóng gói cả API và giao diện;
không dùng static hosting cho backend này.

## Render: demo miễn phí

1. Đưa các thay đổi code lên repo GitHub của bạn. Bao gồm Dockerfile, .dockerignore,
   render.yaml, thư mục main (trừ main/build đã được ignore), và các thay đổi source C++ liên quan.
   Repo hiện tại là https://github.com/mthuw34/DSA_FINAL.
2. Đăng nhập https://dashboard.render.com, chọn **New → Blueprint** và kết nối repo.
3. Dùng Blueprint Path `render.yaml`. Kiểm tra service sử dụng **Free** trước khi triển khai.
4. Khi Render hỏi `HOSPITAL_PASSWORD`, đặt mật khẩu riêng dài ít nhất 12 ký tự.
   Không viết mật khẩu vào repo hoặc gửi trong chat.
5. Bấm Deploy/Apply. Render sẽ build Dockerfile; sau khi Live, mở URL HTTPS mà Render cấp.
6. Trình duyệt hỏi đăng nhập: tên **admin**, mật khẩu bạn đặt ở bước 4.
7. Gửi URL và mật khẩu cho những người được phép dùng. Các thao tác sửa dữ liệu
   của mọi người cùng sử dụng một database của service này.

Địa chỉ chính xác do Render cấp sau khi triển khai; không thể lấy địa chỉ localhost để gửi cho người ngoài.
HTTPS được Render xử lý trước khi chuyển request đến container.

**Free chỉ dành cho demo:** service có thể ngủ khi không hoạt động; dữ liệu SQLite có thể
bị mất khi khởi động lại/redeploy. Lần chạy cloud đầu tiên dùng database trống;
các database trên máy bạn được loại khỏi Docker image. Thêm hồ sơ thử nghiệm trên web để demo.

Tài liệu: [Docker](https://render.com/docs/docker),
[web service và port](https://render.com/docs/web-services),
[giới hạn Free](https://render.com/docs/free).

## Giữ dữ liệu lâu dài trên Render

Dùng Blueprint Path `main/deploy/render-persistent.yaml` thay cho bản Free.
Đây là cấu hình **có phí**, gồm web service và persistent disk 1 GB tại `/data`.
Kiểm tra giá hiện trên dashboard và chỉ triển khai sau khi bạn chấp nhận chi phí.
SQLite của cả bốn module được lưu trên cùng ổ. Duy trì một service instance cho bộ database này.

[Persistent disk](https://render.com/docs/disks) chỉ có trên dịch vụ trả phí;
dữ liệu chỉ bền khi được ghi trong thư mục mount `/data`.

## Chạy Docker trên máy chủ riêng

Trên máy có Docker, từ thư mục gốc repo:

```sh
docker build -t mediflow .
docker volume create mediflow-data
# Đặt HOSPITAL_PASSWORD trong môi trường của shell, không đưa giá trị vào lịch sử lệnh.
docker run -d --name mediflow --restart unless-stopped \
  --env HOSPITAL_PASSWORD \
  -p 127.0.0.1:18080:18080 \
  -v mediflow-data:/data mediflow
```

Đặt reverse proxy HTTPS trước `127.0.0.1:18080` để có domain truy cập Internet.
Đăng nhập HTTP Basic dùng tài khoản chung `admin`; chưa có phân quyền cho từng bác sĩ/nhân viên.
Sao lưu dữ liệu bằng SQLite backup hoặc khi server đã dừng để tránh bản sao không nhất quán.

## Kiểm tra bản mật khẩu trên Windows

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File main/build.ps1 -OutputName hospital_web_deploy.exe
# Đặt HOSPITAL_PASSWORD qua cấu hình môi trường; phải dài ít nhất 12 ký tự.
.\main\build\hospital_web_deploy.exe
```

Biến môi trường `PORT` cấu hình cổng; tham số cổng trên command line được ưu tiên.
`HOSPITAL_BIND` mặc định là `127.0.0.1`. Docker đặt `0.0.0.0` và bắt buộc mật khẩu.
`/api/health` được phép đọc không cần mật khẩu để nền tảng kiểm tra service;
giao diện và mọi API dữ liệu đều yêu cầu mật khẩu khi đã cấu hình.
