#ifndef PATIENT_LOOKUP_H
#define PATIENT_LOOKUP_H

#include <string>
#include <sqlite3.h>

struct Patient
{
    int id;

    std::string name;
    int age;

    std::string phone;
    std::string birthDate;
    std::string gender;
    std::string hometown;
    std::string address;

    double height;
    double weight;
    double bmi;
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