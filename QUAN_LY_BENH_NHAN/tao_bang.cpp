#include <iostream>
#include <sqlite3.h>

int main()
{
    sqlite3* db = nullptr;

    // Mo database
    if (sqlite3_open("hospital.db", &db) != SQLITE_OK)
    {
        std::cerr << "Khong mo duoc database: "
                  << sqlite3_errmsg(db) << '\n';

        sqlite3_close(db);
        return 1;
    }

    // =========================================
    // TAO BANG PATIENTS
    // =========================================

    const char* sqlPatients = R"(

        CREATE TABLE IF NOT EXISTS patients (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            birth_date TEXT NOT NULL,
            age INTEGER NOT NULL,
            gender TEXT,
            hometown TEXT,
            address TEXT,
            phone TEXT
        );

    )";

    char* error = nullptr;

    int result = sqlite3_exec(
        db,
        sqlPatients,
        nullptr,
        nullptr,
        &error
    );

    if (result != SQLITE_OK)
    {
        std::cerr << "Loi tao bang patients: "
                  << error << '\n';

        sqlite3_free(error);
        sqlite3_close(db);

        return 1;
    }

    std::cout << "Tao bang patients thanh cong\n";

    // =========================================
    // TAO BANG CHECKINS
    // =========================================

    const char* sqlCheckin = R"(

        CREATE TABLE IF NOT EXISTS checkins (
            checkin_id INTEGER PRIMARY KEY AUTOINCREMENT,

            patient_id INTEGER NOT NULL,

            department TEXT NOT NULL,

            checkin_time TEXT NOT NULL
                DEFAULT (datetime('now', 'localtime')),

            priority INTEGER NOT NULL
                CHECK(priority BETWEEN 1 AND 5),

            FOREIGN KEY(patient_id)
                REFERENCES patients(id)
        );

    )";

    error = nullptr;

    result = sqlite3_exec(
        db,
        sqlCheckin,
        nullptr,
        nullptr,
        &error
    );

    if (result != SQLITE_OK)
    {
        std::cerr << "Loi tao bang checkins: "
                  << error << '\n';

        sqlite3_free(error);
        sqlite3_close(db);

        return 1;
    }

    std::cout << "Tao bang checkins thanh cong\n";

    // BANG dung chung cho cac phan truy xuat du lieu benh nhan.
    const char* sqlRetrievalQueue = R"(

        CREATE TABLE IF NOT EXISTS retrieval_queue (
            checkin_id INTEGER PRIMARY KEY,
            patient_id INTEGER NOT NULL,
            patient_name TEXT NOT NULL,
            birth_date TEXT NOT NULL,
            age INTEGER NOT NULL,
            gender TEXT,
            hometown TEXT,
            address TEXT,
            phone TEXT,
            department TEXT NOT NULL,
            checkin_time TEXT NOT NULL,
            priority INTEGER NOT NULL,
            FOREIGN KEY(patient_id) REFERENCES patients(id)
        );

        DELETE FROM retrieval_queue;

        INSERT INTO retrieval_queue (
            checkin_id, patient_id, patient_name, birth_date, age,
            gender, hometown, address, phone, department,
            checkin_time, priority
        )
        SELECT
            c.checkin_id,
            c.patient_id,
            p.name AS patient_name,
            p.birth_date,
            p.age,
            p.gender,
            p.hometown,
            p.address,
            p.phone,
            c.department,
            c.checkin_time,
            c.priority
        FROM checkins AS c
        JOIN patients AS p
            ON p.id = c.patient_id;

        CREATE TRIGGER IF NOT EXISTS retrieval_queue_after_checkin_insert
        AFTER INSERT ON checkins
        BEGIN
            INSERT INTO retrieval_queue
            SELECT NEW.checkin_id, NEW.patient_id, p.name, p.birth_date, p.age,
                   p.gender, p.hometown, p.address, p.phone,
                   NEW.department, NEW.checkin_time, NEW.priority
            FROM patients AS p
            WHERE p.id = NEW.patient_id;
        END;

        CREATE TRIGGER IF NOT EXISTS retrieval_queue_after_checkin_update
        AFTER UPDATE ON checkins
        BEGIN
            UPDATE retrieval_queue
            SET patient_id = NEW.patient_id,
                patient_name = (SELECT name FROM patients WHERE id = NEW.patient_id),
                birth_date = (SELECT birth_date FROM patients WHERE id = NEW.patient_id),
                age = (SELECT age FROM patients WHERE id = NEW.patient_id),
                gender = (SELECT gender FROM patients WHERE id = NEW.patient_id),
                hometown = (SELECT hometown FROM patients WHERE id = NEW.patient_id),
                address = (SELECT address FROM patients WHERE id = NEW.patient_id),
                phone = (SELECT phone FROM patients WHERE id = NEW.patient_id),
                department = NEW.department,
                checkin_time = NEW.checkin_time,
                priority = NEW.priority
            WHERE checkin_id = NEW.checkin_id;
        END;

        CREATE TRIGGER IF NOT EXISTS retrieval_queue_after_checkin_delete
        AFTER DELETE ON checkins
        BEGIN
            DELETE FROM retrieval_queue WHERE checkin_id = OLD.checkin_id;
        END;

        CREATE TRIGGER IF NOT EXISTS retrieval_queue_after_patient_update
        AFTER UPDATE ON patients
        BEGIN
            UPDATE retrieval_queue
            SET patient_name = NEW.name,
                birth_date = NEW.birth_date,
                age = NEW.age,
                gender = NEW.gender,
                hometown = NEW.hometown,
                address = NEW.address,
                phone = NEW.phone
            WHERE patient_id = NEW.id;
        END;

    )";

    error = nullptr;

    result = sqlite3_exec(
        db,
        sqlRetrievalQueue,
        nullptr,
        nullptr,
        &error
    );

    if (result != SQLITE_OK)
    {
        std::cerr << "Loi tao view retrieval_queue: "
                  << error << '\n';

        sqlite3_free(error);
        sqlite3_close(db);

        return 1;
    }

    std::cout << "Tao view retrieval_queue thanh cong\n";

    sqlite3_close(db);

    return 0;
}