#pragma once
#include <string>
using namespace std;

struct checkoutTime
{
    int year;
    int month;
    int date;
    int hour;
    int minute;
    int second;
};

struct Patient
{
    string id;
    string name;
    int birthYear;
    string phone;
    string gender;
    string address;
    string lastDiagnosis;       // chẩn đoán, thuốc vừa kê
    checkoutTime time;
};

