const Database = require('better-sqlite3');
const path = require('path');

// Tự động tạo file hospital.db tại thư mục database
const db = new Database(path.join(__dirname, 'hospital.db'));
db.pragma('journal_mode = WAL');

// 1. Câu lệnh tạo bảng (chính là bảng trong ảnh)
db.exec(`
  CREATE TABLE IF NOT EXISTS doctor_schedules (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    doctor_id INTEGER NOT NULL,
    day_of_week INTEGER NOT NULL,
    start_time TEXT NOT NULL,
    end_time TEXT NOT NULL,
    active INTEGER DEFAULT 1
  );
`);

// 2. Chèn sẵn một số dòng dữ liệu mẫu để bảng hiển thị giống ảnh
const insert = db.prepare(`
  INSERT INTO doctor_schedules (doctor_id, day_of_week, start_time, end_time, active)
  VALUES (?, ?, ?, ?, ?)
`);

const insertData = db.transaction(() => {
  insert.run(1, 2, '00:00', '08:00', 1);
  insert.run(1, 3, '00:00', '08:00', 1);
  insert.run(1, 4, '00:00', '08:00', 1);
  insert.run(2, 0, '08:00', '16:00', 1);
  insert.run(2, 1, '08:00', '16:00', 1);
  insert.run(3, 0, '16:00', '23:59', 1);
});

insertData();
console.log('Tạo database và bảng thành công!');