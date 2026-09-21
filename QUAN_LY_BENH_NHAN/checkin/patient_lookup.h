#ifndef PATIENT_LOOKUP_H
#define PATIENT_LOOKUP_H

#include <string>
#include <sqlite3.h>

struct Patient
{
    int id;

    std::string name;
    std::string birthDate;

    int age;

    std::string gender;
    std::string hometown;
    std::string address;
    std::string phone;
};

bool timBenhNhan(
    sqlite3* db,
    int patientId,
    Patient& patient
);

void hienThiBenhNhan(
    const Patient& patient
);

#endif