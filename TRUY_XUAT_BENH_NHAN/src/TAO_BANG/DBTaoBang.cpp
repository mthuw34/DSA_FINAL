// Khai báo lớp DBTaoBang, nơi cung cấp thao tác tạo các bảng hàng đợi truy xuất.
#include "TaoBangTruyXuat.h"
// Nạp cấu hình khoa và các kiểu dữ liệu dùng chung của chức năng truy xuất.
#include "../TRUY_XUAT/TruyXuat.h"

// Dùng các tiện ích nhập/xuất chuẩn như cout và cerr.
#include <iostream>
// Dùng API SQLite để mở cơ sở dữ liệu và thực thi câu lệnh SQL.
#include <sqlite3.h>
// Dùng kiểu chuỗi để ghép tên bảng vào câu lệnh SQL.
#include <string>

// Cho phép gọi các thành phần thư viện chuẩn mà không cần viết tiền tố std::.
using namespace std;

// Các thành phần trong namespace này chỉ được dùng bên trong tệp hiện tại.
namespace {

// Đường dẫn tương đối tới cơ sở dữ liệu chứa các bảng hàng đợi truy xuất.
// Chương trình cần chạy từ thư mục gốc dự án để đường dẫn này được phân giải đúng.
constexpr const char* retrievalPath = "TRUY_XUAT_BENH_NHAN/db/truyXuat.db";

// Thực thi một câu lệnh SQL không cần nhận từng dòng kết quả trả về.
bool executeSql(sqlite3* database, const string& sql, const char* operation) 
    {
        // SQLite sẽ cấp phát chuỗi lỗi tại đây nếu câu lệnh thất bại.
        char* errorMessage = nullptr;
        // Chạy câu SQL; không truyền callback vì thao tác này không đọc các dòng kết quả.
        if (sqlite3_exec(database, sql.c_str(), nullptr, nullptr, &errorMessage) == SQLITE_OK) {
            // Báo cho bên gọi biết câu lệnh đã chạy thành công.
            return true;
        }

        // In tên thao tác và thông tin lỗi do SQLite cung cấp ra luồng lỗi chuẩn.
        cerr << operation << ": "
                << (errorMessage ? errorMessage : sqlite3_errmsg(database)) << '\n';
        // Giải phóng chuỗi lỗi do sqlite3_exec cấp phát để tránh rò rỉ bộ nhớ.
        sqlite3_free(errorMessage);
        // Báo cho bên gọi biết câu lệnh SQL đã thất bại.
        return false;
    }

}

// Tạo các bảng hàng đợi truy xuất, mỗi khoa được cấu hình có một bảng riêng.
bool DBTaoBang::taoBangTruyXuat() 
{
    // Con trỏ kết nối chưa trỏ tới cơ sở dữ liệu nào.
    sqlite3* database = nullptr;
    // Mở cơ sở dữ liệu để đọc/ghi; nếu chưa tồn tại thì tạo tệp cơ sở dữ liệu mới.
    if (sqlite3_open_v2(
            retrievalPath,
            &database,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
            nullptr
        ) != SQLITE_OK) 
        {
            // Thông báo lỗi mở tệp; nếu chưa có kết nối thì dùng thông báo tổng quát.
            cerr << "Khong mo duoc truyXuat.db: "
                    << (database ? sqlite3_errmsg(database) : "loi SQLite") << '\n';
            // Đóng kết nối nếu SQLite đã tạo được đối tượng kết nối trước khi báo lỗi.
            if (database) sqlite3_close(database);
            // Dừng thao tác vì không thể tạo bảng khi chưa mở được cơ sở dữ liệu.
            return false;
        }

    // Bắt đầu giao dịch để các thao tác tạo bảng được xử lý trong cùng một giao dịch.
    if (!executeSql(database, "BEGIN TRANSACTION;", "Khong the bat dau giao dich tao bang")) {
        // Đóng cơ sở dữ liệu nếu không thể bắt đầu giao dịch.
        sqlite3_close(database);
        // Báo thất bại để bên gọi biết thao tác khởi tạo không hoàn tất.
        return false;
    }

    // Mặc định coi quá trình tạo bảng thành công; sẽ đổi thành false khi gặp lỗi.
    bool success = true;
    // Duyệt lần lượt danh sách khoa dùng chung của chức năng truy xuất.
    for (const auto& department : CauHinhTruyXuat::danhSachKhoa) {
        // Tạo câu lệnh tạo bảng với tên lấy từ cấu hình của khoa hiện tại.
        // IF NOT EXISTS giúp có thể chạy lại chương trình mà không ghi đè bảng đã có.
        const string createSql =
            "CREATE TABLE IF NOT EXISTS " + string(department.tenBang) + R"( (
                -- Số thứ tự của bệnh nhân trong hàng đợi; đồng thời là khóa chính.
                retrieval_order INTEGER PRIMARY KEY,
                -- Mã lượt khám phải có giá trị và không được trùng trong cùng bảng.
                checkin_id INTEGER NOT NULL UNIQUE,
                -- Mã bệnh nhân, bắt buộc có giá trị.
                patient_id INTEGER NOT NULL,
                -- Tên khoa lưu bệnh nhân trong hàng đợi, bắt buộc có giá trị.
                department TEXT NOT NULL,
                -- Thời điểm bệnh nhân đăng ký khám, bắt buộc có giá trị.
                checkin_time TEXT NOT NULL,
                -- Mức ưu tiên ban đầu của lượt khám, bắt buộc có giá trị.
                base_priority INTEGER NOT NULL,
                -- Mức ưu tiên hiện tại sau các lần cập nhật, bắt buộc có giá trị.
                current_priority INTEGER NOT NULL,
                -- Thời điểm cập nhật gần nhất; có thể để trống nếu chưa cập nhật.
                last_update TEXT
            );)";
        // Tạo bảng; nếu thất bại thì ghi nhận lỗi và dừng vòng lặp để rollback.
        if (!executeSql(database, createSql, "Loi tao bang truy xuat")) {
            // Đánh dấu giao dịch không thể hoàn tất trọn vẹn.
            success = false;
            // Không tạo tiếp các bảng còn lại sau khi đã phát hiện lỗi.
            break;
        }
    }

    // Chỉ xác nhận giao dịch khi mọi bảng đều tạo được và lệnh COMMIT thành công.
    if (success && executeSql(database, "COMMIT;", "Khong the hoan tat giao dich tao bang")) {
        // Đóng kết nối sau khi dữ liệu đã được xác nhận lưu thành công.
        sqlite3_close(database);
        // Thông báo thành công cho người dùng hoặc tiến trình gọi.
        cout << "Tao bang truy xuat thanh cong.\n";
        // Trả kết quả thành công cho bên gọi.
        return true;
    }

    // Hủy các thay đổi trong giao dịch nếu tạo bảng hoặc COMMIT thất bại.
    executeSql(database, "ROLLBACK;", "Khong the rollback giao dich tao bang");
    // Đóng kết nối sau khi đã thử hoàn tác giao dịch.
    sqlite3_close(database);
    // Báo cho bên gọi biết quá trình tạo các bảng không hoàn tất.
    return false;
}
