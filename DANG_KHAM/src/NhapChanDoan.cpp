#pragma once
#include <string>
#include <vector>
#include "patient.h"
#include <iostream>
using namespace std;

void NhapChanDoan (const string& idBenhNhan, string& chanDoan, string& donThuoc)
{
    cout << "Chan doan cua benh nhan: ";
    getline(cin, chanDoan);
    cin .ignore(); 
    cout << endl;

    cout << "Don thuoc cua benh nhan: ";
    getline(cin, donThuoc);
    cin .ignore();

    return;
}

