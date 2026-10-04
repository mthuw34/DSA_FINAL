#include "NhapChanDoan.h"

#include <iostream>

using namespace std;

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

    getline(
        cin >> ws,
        chanDoan
    );

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
