#pragma once
#include <string>
using namespace std;

struct BenhNhanKham {
    int RetrievalOrder = 0;
    int CheckinId = 0;
    int PatientId = 0;

    string khoa;
    string CheckinTime;
    int BasePriority = 0;
    int CurrentPriority = 0;
    string LastUpdate;

    // Thông tin được bổ sung trong quá trình xử lý khám
    string DoctorId;
    string DoctorName;
    string KhoaBacSi;
    string StartTime;
    string EndTime;
    int ExamDuration = 0;
    string Status;
    string Note;
};