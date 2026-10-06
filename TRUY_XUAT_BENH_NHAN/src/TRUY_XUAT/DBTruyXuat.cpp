// Nạp khai báo hồ sơ bệnh nhân, cấu hình khoa và giao diện thao tác CSDL.
#include "TruyXuat.h"

// Dùng mảng cố định chứa câu lệnh SQLite và bộ đếm theo khoa.
#include <array>
// Dùng luồng lỗi chuẩn để báo lỗi cơ sở dữ liệu.
#include <iostream>
// Dùng API SQLite để đọc và ghi cơ sở dữ liệu.
#include <sqlite3.h>
// Dùng std::string để ghép câu lệnh SQL với tên bảng cấu hình.
#include <string>

// Cho phép gọi các thành phần thư viện chuẩn mà không cần tiền tố std::.
using namespace std;

// Các hàm và hằng số này chỉ được sử dụng trong tệp hiện tại.
namespace {

// Đường dẫn tương đối tới cơ sở dữ liệu chứa dữ liệu ưu tiên đầu vào.
constexpr const char* priorityPath = "THAY_DOI_MUC_DO_UU_TIEN/db/priority.db";
// Đường dẫn tương đối tới cơ sở dữ liệu chứa các hàng đợi kết quả.
constexpr const char* retrievalPath = "TRUY_XUAT_BENH_NHAN/db/truyXuat.db";

// In tên thao tác và thông tin lỗi hiện tại của SQLite.
void reportError(sqlite3* database, const char* operation) {
    cerr << operation << ": "
              << (database ? sqlite3_errmsg(database) : "loi SQLite") << '\n';
}

// Chạy câu lệnh SQL không cần đọc từng dòng kết quả và trả trạng thái thành công.
bool executeSql(sqlite3* database, const char* sql, const char* operation) {
    // SQLite ghi nội dung lỗi vào con trỏ này nếu thực thi thất bại.
    char* errorMessage = nullptr;
    // Thực thi câu lệnh; không cần callback vì đây không phải truy vấn lấy dữ liệu.
    if (sqlite3_exec(database, sql, nullptr, nullptr, &errorMessage) == SQLITE_OK) {
        return true;
    }

    // In chi tiết lỗi do SQLite cung cấp, hoặc thông báo lỗi trên kết nối.
    cerr << operation << ": "
              << (errorMessage ? errorMessage : sqlite3_errmsg(database)) << '\n';
    // Giải phóng chuỗi lỗi do sqlite3_exec cấp phát.
    sqlite3_free(errorMessage);
    // Báo cho bên gọi biết câu lệnh đã thất bại.
    return false;
}

}

// Đọc các lượt khám từ priority.db và đưa từng hồ sơ vào mảng đầu ra.
bool DBTruyXuat::docDanhSachBenhNhan(MangDongBenhNhan& outRecords) {
    // Bảo đảm mảng đầu ra chỉ chứa dữ liệu của lần đọc hiện tại.
    outRecords.clear();

    // Khởi tạo con trỏ kết nối ở trạng thái chưa mở.
    sqlite3* database = nullptr;
    // Mở CSDL nguồn chỉ đọc để tránh thay đổi dữ liệu ưu tiên gốc.
    if (sqlite3_open_v2(priorityPath, &database, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        // Báo lỗi mở CSDL và giải phóng kết nối nếu SQLite đã tạo đối tượng.
        reportError(database, "Khong mo duoc priority.db");
        if (database) sqlite3_close(database);
        return false;
    }

    // Lấy các trường cần cho sắp xếp và ghi hàng đợi truy xuất.
    constexpr const char* query = R"(
        SELECT checkin_id, patient_id, department, checkin_time,
               base_priority, current_priority, last_update
        FROM priority_checkins;
    )";
    // Câu lệnh đã biên dịch sẽ được dùng để đọc từng dòng kết quả.
    sqlite3_stmt* statement = nullptr;
    // Chuẩn bị câu truy vấn trước khi bắt đầu lấy dữ liệu.
    if (sqlite3_prepare_v2(database, query, -1, &statement, nullptr) != SQLITE_OK) {
        // Báo lỗi truy vấn, đóng CSDL và kết thúc thao tác.
        reportError(database, "Khong the chuan bi truy van priority.db");
        sqlite3_close(database);
        return false;
    }

    // Khởi tạo mã trạng thái để vòng lặp gọi sqlite3_step() lần đầu.
    int result = SQLITE_ROW;
    // Mỗi lần sqlite3_step trả SQLITE_ROW thì đang có một dòng kết quả mới.
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        // Khởi tạo hồ sơ với giá trị mặc định cho các trường chưa được nạp.
        HoSoTruyXuat record{};
        // Đọc mã lượt khám từ cột đầu tiên.
        record.checkinId = sqlite3_column_int(statement, 0);
        // Đọc mã bệnh nhân từ cột thứ hai.
        record.patientId = sqlite3_column_int(statement, 1);

        // Lấy tên khoa dạng văn bản; SQLite có thể trả con trỏ null.
        const auto* departmentText = sqlite3_column_text(statement, 2);
        // Chuyển tên khoa sang std::string, hoặc dùng chuỗi rỗng nếu là NULL.
        record.department = departmentText
            ? reinterpret_cast<const char*>(departmentText)
            : "";
        // Ánh xạ tên khoa sang số thứ tự dùng bởi sắp xếp và bảng đích.
        record.departmentOrder = CauHinhTruyXuat::thuTuKhoa(record.department);

        // Lấy thời điểm đăng ký khám dạng văn bản.
        const auto* checkinTimeText = sqlite3_column_text(statement, 3);
        // Chuyển thời điểm sang std::string, hoặc dùng chuỗi rỗng nếu là NULL.
        record.checkinTime = checkinTimeText
            ? reinterpret_cast<const char*>(checkinTimeText)
            : "";
        // Đọc mức ưu tiên ban đầu từ cột thứ năm.
        record.basePriority = sqlite3_column_int(statement, 4);
        // Đọc mức ưu tiên hiện tại từ cột thứ sáu.
        record.currentPriority = sqlite3_column_int(statement, 5);

        // Lấy thời điểm cập nhật cuối cùng; giá trị này có thể là NULL.
        const auto* lastUpdateText = sqlite3_column_text(statement, 6);
        // Chuyển thành chuỗi C++; NULL được biểu diễn bằng chuỗi rỗng trong bộ nhớ.
        record.lastUpdate = lastUpdateText
            ? reinterpret_cast<const char*>(lastUpdateText)
            : "";
        // Thêm hồ sơ vừa đọc vào mảng động đầu ra.
        outRecords.push_back(record);
    }

    // SQLITE_DONE nghĩa là đã đọc hết kết quả mà không gặp lỗi.
    const bool success = result == SQLITE_DONE;
    // Nếu kết thúc bằng trạng thái khác, báo lỗi và bỏ các bản ghi đọc dở.
    if (!success) {
        reportError(database, "Loi doc priority.db");
        outRecords.clear();
    }

    // Giải phóng câu truy vấn đã biên dịch trước khi đóng kết nối.
    sqlite3_finalize(statement);
    // Đóng kết nối CSDL nguồn sau khi hoàn tất đọc.
    sqlite3_close(database);
    // Trả trạng thái đọc cho bên gọi.
    return success;
}

// Thay toàn bộ dữ liệu trong các bảng hàng đợi bằng hồ sơ đã sắp xếp.
bool DBTruyXuat::ghiDanhSachDaSapXep(
    const MangDongBenhNhan& sortedRecords
) {
    // Khởi tạo con trỏ kết nối ở trạng thái chưa mở.
    sqlite3* database = nullptr;
    // Mở CSDL kết quả ở chế độ đọc/ghi; không tự tạo tệp nếu chưa tồn tại.
    if (sqlite3_open_v2(retrievalPath, &database, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        // Báo lỗi mở CSDL đích và đóng kết nối nếu cần.
        reportError(database, "Khong mo duoc truyXuat.db");
        if (database) sqlite3_close(database);
        return false;
    }

    // Bắt đầu giao dịch để xóa cũ và ghi mới như một đơn vị công việc.
    if (!executeSql(database, "BEGIN TRANSACTION;", "Khong the bat dau giao dich ghi")) {
        // Không thể ghi an toàn nếu giao dịch không bắt đầu được.
        sqlite3_close(database);
        return false;
    }

    // Cờ theo dõi lỗi xuyên suốt quá trình xóa và chèn dữ liệu.
    bool success = true;
    // Xóa dữ liệu cũ khỏi từng bảng khoa trước khi dựng lại hàng đợi.
    for (const auto& department : CauHinhTruyXuat::danhSachKhoa) {
        // Tạo câu lệnh xóa bằng tên bảng đã khai báo trong cấu hình.
        const string clearSql =
            "DELETE FROM " + string(department.tenBang) + ";";
        // Nếu xóa một bảng thất bại thì dừng vòng lặp và sẽ rollback.
        if (!executeSql(database, clearSql.c_str(), "Khong the xoa du lieu hang doi cu")) {
            success = false;
            break;
        }
    }

    // Tạo một câu lệnh INSERT đã biên dịch cho mỗi bảng khoa.
    array<sqlite3_stmt*, CauHinhTruyXuat::danhSachKhoa.size()> statements{};
    // Chuẩn bị các câu lệnh chỉ khi bước xóa trước đó chưa gặp lỗi.
    for (size_t index = 0; success && index < statements.size(); ++index) {
        // Các dấu ? được bind giá trị hồ sơ riêng cho mỗi lượt chèn.
        const string insertSql =
            "INSERT INTO " + string(CauHinhTruyXuat::danhSachKhoa[index].tenBang)
            + " VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
        // Biên dịch câu lệnh cho bảng của khoa hiện tại.
        if (sqlite3_prepare_v2(
                database,
                insertSql.c_str(),
                -1,
                &statements[index],
                nullptr
            ) != SQLITE_OK) {
            // Báo lỗi chuẩn bị câu lệnh và đánh dấu quá trình ghi thất bại.
            reportError(database, "Khong the chuan bi cau lenh ghi hang doi");
            success = false;
        }
    }

    // Lưu số thứ tự hàng đợi kế tiếp của từng khoa, bắt đầu từ 1.
    array<int, CauHinhTruyXuat::danhSachKhoa.size()> departmentOrders{};
    // Duyệt các hồ sơ theo thứ tự đã được sắp xếp ở tầng quản lý.
    for (int recordIndex = 0; recordIndex < sortedRecords.size(); ++recordIndex) {
        // Ngừng chèn ngay khi một thao tác trước đó đã thất bại.
        if (!success) break;
        // Lấy hồ sơ hiện tại để xác định bảng và bind các cột.
        const HoSoTruyXuat& record = sortedRecords[recordIndex];
        // Bỏ qua hồ sơ không ánh xạ tới một khoa trong danh sách bảng được hỗ trợ.
        if (record.departmentOrder < 1
            || static_cast<size_t>(record.departmentOrder) > statements.size()) {
            continue;
        }

        // Chuyển số khoa đánh số từ 1 thành chỉ số mảng đánh số từ 0.
        const size_t departmentIndex =
            static_cast<size_t>(record.departmentOrder - 1);
        // Chọn câu lệnh INSERT đã chuẩn bị cho khoa tương ứng.
        sqlite3_stmt* statement = statements[departmentIndex];
        // Đặt lại câu lệnh để sử dụng cho hồ sơ tiếp theo của cùng khoa.
        sqlite3_reset(statement);
        // Xóa các giá trị bind từ lần thực thi trước.
        sqlite3_clear_bindings(statement);
        // Bind số thứ tự hàng đợi tăng riêng trong khoa hiện tại.
        sqlite3_bind_int(statement, 1, ++departmentOrders[departmentIndex]);
        // Bind mã lượt khám.
        sqlite3_bind_int(statement, 2, record.checkinId);
        // Bind mã bệnh nhân.
        sqlite3_bind_int(statement, 3, record.patientId);
        // Bind tên khoa; SQLITE_TRANSIENT yêu cầu SQLite tự sao chép chuỗi.
        sqlite3_bind_text(statement, 4, record.department.c_str(), -1, SQLITE_TRANSIENT);
        // Bind thời điểm đăng ký khám.
        sqlite3_bind_text(statement, 5, record.checkinTime.c_str(), -1, SQLITE_TRANSIENT);
        // Bind mức ưu tiên ban đầu và hiện tại.
        sqlite3_bind_int(statement, 6, record.basePriority);
        sqlite3_bind_int(statement, 7, record.currentPriority);
        // Lưu thời điểm cập nhật thành SQL NULL nếu chuỗi rỗng.
        if (record.lastUpdate.empty()) {
            sqlite3_bind_null(statement, 8);
        } else {
            // Nếu có dữ liệu, bind thời điểm cập nhật dưới dạng văn bản.
            sqlite3_bind_text(statement, 8, record.lastUpdate.c_str(), -1, SQLITE_TRANSIENT);
        }

        // Thực thi INSERT cho hồ sơ hiện tại.
        if (sqlite3_step(statement) != SQLITE_DONE) {
            // Ghi lỗi SQLite và đánh dấu để hủy toàn bộ giao dịch.
            reportError(database, "Loi ghi du lieu hang doi");
            success = false;
        }
    }

    // Giải phóng toàn bộ câu lệnh đã chuẩn bị, kể cả khi có câu lệnh chưa dùng.
    for (sqlite3_stmt* statement : statements) {
        if (statement) sqlite3_finalize(statement);
    }

    // Chỉ xác nhận giao dịch khi mọi bước trước đó thành công.
    if (success && executeSql(database, "COMMIT;", "Khong the hoan tat giao dich ghi")) {
        // Đóng kết nối sau khi dữ liệu mới đã được xác nhận.
        sqlite3_close(database);
        return true;
    }

    // Hủy các thay đổi nếu xóa, chuẩn bị, chèn hoặc xác nhận giao dịch thất bại.
    executeSql(database, "ROLLBACK;", "Khong the rollback giao dich ghi");
    // Đóng kết nối trước khi trả kết quả thất bại.
    sqlite3_close(database);
    return false;
}
