#include "NhapChanDoan.h"

#include <iostream>
#include <limits>

using namespace std;

// Nhập chẩn đoán, đơn thuốc và lời nhắc của bác sĩ từ bàn phím.
void NhapChanDoan(
    const string& idBenhNhan,
    string& chanDoan,
    string& donThuoc,
    string& loiNhacBacSi
)
{
    cout
        << "\nBenh nhan ID: "
        << idBenhNhan
        << '\n';

    cout
        << "Chan doan cua benh nhan: ";

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, chanDoan);

    cout
        << "Don thuoc cua benh nhan: ";

    getline(
        cin,
        donThuoc
    );

    cout
        << "Loi nhac cua bac si: ";

    getline(
        cin,
        loiNhacBacSi
    );
}
