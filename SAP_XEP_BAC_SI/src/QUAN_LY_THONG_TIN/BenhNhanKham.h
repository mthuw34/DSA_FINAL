#pragma once
#include <string>

struct BenhNhanKham {
    int RetrievalOrder = 0;
    int CheckinId = 0;
    int PatientId = 0;

    std::string khoa;
    std::string CheckinTime;
    int BasePriority = 0;
    int CurrentPriority = 0;
    std::string LastUpdate;

    // Thông tin được bổ sung trong quá trình xử lý khám.
    std::string DoctorId;
    std::string DoctorName;
    std::string KhoaBacSi;
    std::string StartTime;
    std::string EndTime;
    int ExamDuration = 0;
    std::string Status;
    std::string Note;
};
