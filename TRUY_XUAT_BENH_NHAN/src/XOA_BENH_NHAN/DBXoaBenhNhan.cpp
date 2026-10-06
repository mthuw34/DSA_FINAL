// Nạp khai báo các hàm xóa ở tầng dữ liệu.
#include "XoaBenhNhan.h"
// Nạp cấu hình khoa và giao diện đọc/ghi hàng đợi truy xuất.
#include "../TRUY_XUAT/TruyXuat.h"
// Nạp cấu trúc mảng động dùng để tìm/xóa hồ sơ trong bộ nhớ.
#include "MangDongBenhNhan.h"

// Dùng luồng nhập/xuất chuẩn để báo trạng thái và lỗi.
#include <iostream>
// Dùng SQLite để cập nhật cơ sở dữ liệu nguồn.
#include <sqlite3.h>
// Dùng chuỗi cho các câu lệnh SQLite.
#include <string>

// Cho phép dùng cerr/cout mà không cần tiền tố std::.
using namespace std;

// Các hàm và hằng số sau đây chỉ được dùng trong tệp này.
namespace {

// Đường dẫn tương đối tới cơ sở dữ liệu chứa các lượt khám nguồn.
constexpr const char* priorityPath = "THAY_DOI_MUC_DO_UU_TIEN/db/priority.db";

// Thực thi câu lệnh SQL và in lỗi cụ thể nếu SQLite trả về thất bại.
bool executeSql(sqlite3* database, const char* sql, const char* operation) {
    // SQLite cấp phát thông báo lỗi vào con trỏ này khi có lỗi.
    char* errorMessage = nullptr;
    // Thực thi SQL không cần nhận kết quả dạng từng dòng.
    if (sqlite3_exec(database, sql, nullptr, nullptr, &errorMessage) == SQLITE_OK) {
        return true;
    }
    // In tên thao tác và chi tiết lỗi từ SQLite.
    cerr << operation << ": "
         << (errorMessage ? errorMessage : sqlite3_errmsg(database)) << '\n';
    // Giải phóng chuỗi lỗi do sqlite3_exec cấp phát.
    sqlite3_free(errorMessage);
    return false;
}

// Ghi lại toàn bộ danh sách hồ sơ hiện có vào bảng nguồn priority_checkins.
bool ghiDanhSachPriorityDB(const MangDongBenhNhan& records) {
    // Khởi tạo con trỏ kết nối trước khi mở cơ sở dữ liệu.
    sqlite3* database = nullptr;
    // Mở cơ sở dữ liệu nguồn ở chế độ đọc/ghi.
    if (sqlite3_open_v2(priorityPath, &database, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        // Báo lỗi mở CSDL, đóng kết nối nếu SQLite đã tạo đối tượng kết nối.
        cerr << "Khong mo duoc priority.db: "
             << (database ? sqlite3_errmsg(database) : "loi SQLite") << '\n';
        if (database) sqlite3_close(database);
        return false;
    }

    // Chờ tối đa 5 giây nếu cơ sở dữ liệu đang bị kết nối khác khóa.
    sqlite3_busy_timeout(database, 5000);
    // Bắt đầu giao dịch ghi độc quyền sớm để tránh xung đột cập nhật.
    if (!executeSql(database, "BEGIN IMMEDIATE;", "Loi bat dau cap nhat priority.db")) {
        // Đóng kết nối vì không thể tiếp tục an toàn nếu giao dịch không bắt đầu.
        sqlite3_close(database);
        return false;
    }

    // Xóa các dòng hiện tại; danh sách mới sẽ được chèn trong cùng giao dịch.
    bool success = executeSql(database, "DELETE FROM priority_checkins;", "Loi xoa du lieu cu");
    // Con trỏ câu lệnh INSERT, ban đầu chưa được chuẩn bị.
    sqlite3_stmt* statement = nullptr;
    // Câu lệnh có tham số để ghi từng hồ sơ bằng bind, không ghép giá trị vào SQL.
    const char* insertSql = R"(
        INSERT INTO priority_checkins
            (checkin_id, patient_id, department, checkin_time,
             base_priority, current_priority, last_update)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )";
    // Chỉ chuẩn bị câu lệnh khi xóa dữ liệu cũ thành công.
    if (success && sqlite3_prepare_v2(database, insertSql, -1, &statement, nullptr) != SQLITE_OK) {
        // Báo lỗi chuẩn bị câu lệnh và ngăn các bước ghi tiếp theo.
        cerr << "Loi chuan bi ghi priority.db: " << sqlite3_errmsg(database) << '\n';
        success = false;
    }

    // Duyệt và ghi lần lượt từng hồ sơ khi các thao tác trước đó còn thành công.
    for (int index = 0; success && index < records.size(); ++index) {
        // Lấy tham chiếu đến hồ sơ hiện tại trong mảng.
        const auto& record = records[index];
        // Gán mã lượt khám và mã bệnh nhân vào hai tham số đầu.
        sqlite3_bind_int(statement, 1, record.checkinId);
        sqlite3_bind_int(statement, 2, record.patientId);
        // Gán khoa và thời điểm khám; SQLITE_TRANSIENT yêu cầu SQLite sao chép chuỗi.
        sqlite3_bind_text(statement, 3, record.department.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 4, record.checkinTime.c_str(), -1, SQLITE_TRANSIENT);
        // Gán mức ưu tiên ban đầu và mức ưu tiên hiện tại.
        sqlite3_bind_int(statement, 5, record.basePriority);
        sqlite3_bind_int(statement, 6, record.currentPriority);
        // Lưu giá trị rỗng thành SQL NULL; nếu có nội dung thì lưu dạng văn bản.
        if (record.lastUpdate.empty())
            sqlite3_bind_null(statement, 7);
        else
            sqlite3_bind_text(statement, 7, record.lastUpdate.c_str(), -1, SQLITE_TRANSIENT);

        // Thực thi câu lệnh INSERT cho hồ sơ hiện tại.
        if (sqlite3_step(statement) != SQLITE_DONE) {
            // Báo lỗi ghi và dừng chèn các hồ sơ còn lại.
            cerr << "Loi ghi priority.db: " << sqlite3_errmsg(database) << '\n';
            success = false;
            break;
        }
        // Đặt câu lệnh về trạng thái ban đầu để dùng lại cho hồ sơ kế tiếp.
        sqlite3_reset(statement);
        // Xóa các tham số đã bind trước khi bind giá trị mới.
        sqlite3_clear_bindings(statement);
    }

    // Giải phóng câu lệnh nếu đã được chuẩn bị.
    sqlite3_finalize(statement);
    // Xác nhận giao dịch khi mọi thao tác xóa và chèn thành công.
    if (success)
        success = executeSql(database, "COMMIT;", "Loi hoan tat cap nhat priority.db");
    // Hủy các thay đổi nếu có lỗi trong quá trình cập nhật hoặc xác nhận.
    if (!success)
        executeSql(database, "ROLLBACK;", "Loi rollback priority.db");
    // Đóng kết nối sau khi hoàn tất hoặc hoàn tác giao dịch.
    sqlite3_close(database);
    // Trả trạng thái cập nhật cơ sở dữ liệu nguồn.
    return success;
}

// Kết thúc namespace riêng của các tiện ích nội bộ trong tệp.
} // end namespace

// Xóa lượt khám đầu tiên tìm thấy có mã bệnh nhân bằng patientId.
bool DBXoaBenhNhan::xoaBenhNhan(int patientId) {
    // Từ chối mã không hợp lệ trước khi truy cập cơ sở dữ liệu.
    if (patientId <= 0) return false;

    // Đọc danh sách lượt khám nguồn vào mảng động để thao tác theo cấu trúc dữ liệu.
    MangDongBenhNhan records;
    // Dừng nếu không lấy được dữ liệu ban đầu.
    if (!DBTruyXuat::docDanhSachBenhNhan(records)) {
        cerr << "Khong the doc CSDL de xoa.\n";
        return false;
    }

    // Theo dõi xem có tìm thấy lượt khám thuộc bệnh nhân cần xóa hay không.
    bool found = false;
    // Tìm tuần tự trong mảng vì dữ liệu đã được nạp vào bộ nhớ.
    for (int i = 0; i < records.size(); ++i) {
        // So sánh mã bệnh nhân trong hồ sơ hiện tại với mã cần xóa.
        if (records[i].patientId == patientId) {
            // Xóa phần tử và dồn các hồ sơ phía sau sang trái.
            records.erase(i);
            // Ghi nhận đã tìm thấy và xóa một lượt khám.
            found = true;
            // Dừng sau lượt đầu tiên có mã bệnh nhân tương ứng.
            break;
        }
    }

    // Không ghi lại cơ sở dữ liệu nếu không tìm thấy hồ sơ phù hợp.
    if (!found) {
        cout << "Khong tim thay benh nhan ID " << patientId << " de xoa.\n";
        return false;
    }

    // Tạo bộ đệm và sao chép danh sách còn lại để sắp xếp trước khi lưu hàng đợi.
    MangDongBenhNhan buffer;
    // Sao chép lần lượt hồ sơ còn lại vào bộ đệm.
    for (int i = 0; i < records.size(); ++i)
        buffer.push_back(records[i]);
    // Sắp xếp lại danh sách nếu sau khi xóa vẫn còn hồ sơ.
    if (!records.empty())
        ThuatToanSapXep::sapXepTron(records, buffer, 0, records.size() - 1);

    // Cập nhật CSDL nguồn trước; nếu thất bại thì không ghi lại hàng đợi đích.
    if (!ghiDanhSachPriorityDB(records)) return false;
    // Ghi lại các bảng hàng đợi sau khi CSDL nguồn đã được cập nhật thành công.
    const bool success = DBTruyXuat::ghiDanhSachDaSapXep(records);
    // Báo thành công hoặc báo rõ tình trạng hai bước cập nhật không đồng nhất.
    if (success) {
        cout << "Da xoa benh nhan ID " << patientId << " thanh cong.\n";
    } else {
        cerr << "Da cap nhat priority.db nhung khong cap nhat duoc hang doi truy xuat.\n";
    }
    // Trả kết quả cập nhật hàng đợi cho bên gọi.
    return success;
}

// Xóa toàn bộ lượt khám khỏi cơ sở dữ liệu nguồn và các bảng hàng đợi.
bool DBXoaBenhNhan::xoaTatCaBenhNhan() {
    // Mảng rỗng biểu diễn trạng thái không còn hồ sơ nào cần lưu.
    MangDongBenhNhan emptyRecords;

    // Thay nội dung CSDL nguồn bằng danh sách rỗng.
    const bool p_success = ghiDanhSachPriorityDB(emptyRecords);
    // Thay nội dung các bảng hàng đợi dẫn xuất bằng danh sách rỗng.
    const bool r_success = DBTruyXuat::ghiDanhSachDaSapXep(emptyRecords);

    // Chỉ báo thành công nếu cả hai lần cập nhật đều thành công.
    if (p_success && r_success) {
        cout << "Da xoa toan bo benh nhan.\n";
        return true;
    }
    // Báo thất bại nếu một trong hai cơ sở dữ liệu không cập nhật được.
    return false;
}