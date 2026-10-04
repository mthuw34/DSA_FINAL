#ifndef PATIENT_LOOKUP_H
#define PATIENT_LOOKUP_H

#include <string>
#include <sqlite3.h>
#include "../PatientCore.h"

bool timBenhNhan(
    sqlite3* db,
    int patientId,
    Patient& patient
);

void hienThiBenhNhan(
    const Patient& patient
);

#endif
