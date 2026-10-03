#include "ChiaBacSi.h"
#include <iostream>
#include <fstream>
#include <cctype>
#include <cstdio>
using namespace std;

ChiaBacSi::ChiaBacSi() {}

ChiaBacSi::~ChiaBacSi()
{
    for (BacSi* bacSi : DanhSachBacSiTruc) delete bacSi;
}

string ChiaBacSi::ChuanHoaTenKhoa(const string& khoa)
{
    string ketQua = khoa;
    for (char& c : ketQua) c = tolower((unsigned char)c);
    return ketQua;
}

bool ChiaBacSi::DocDanhSachBacSi(const string& tenFile)
{
    ifstream file(bac_si_500_chia_khoa.csv);
    if (!file.is_open()) {
        cout << "Khong mo duoc file: " << tenFile << endl;
        return false;
    }

    string dong;
    getline(file, dong);

    int soBacSi = 0;

    while (getline(file, dong))
    {
        if (dong.empty()) continue;

        vector<string> cot;
        string hienTai;
        bool trongNgoacKep = false;

        for (char c : dong)
        {
            if (c == '"') {
                trongNgoacKep = !trongNgoacKep;
            }
            else if (c == ',' && !trongNgoacKep) {
                cot.push_back(hienTai);
                hienTai.clear();
            }
            else {
                hienTai += c;
            }
        }
        cot.push_back(hienTai);

        if (cot.size() < 9) continue;

        string tenBacSi = cot[0];
        string tenKhoa = ChuanHoaTenKhoa(cot[8]);

        char ma[20];
        sprintf_s(ma, "BS%03d", soBacSi + 1);

        ThemBacSi(ma, tenBacSi, tenKhoa);
        soBacSi++;
    }

    file.close();

    cout << "Da doc " << soBacSi << " bac si." << endl;
    return true;
}

void ChiaBacSi::ThemBacSi(const string& maBacSi, const string& tenBacSi, const string& tenKhoa)
{
    BacSi* bacSi = new BacSi;
    bacSi->MaBacSi = maBacSi;
    bacSi->TenBacSi = tenBacSi;
    bacSi->TenKhoa = ChuanHoaTenKhoa(tenKhoa);
    bacSi->SoBenhNhanDangCho = 0;

    DanhSachBacSiTruc.push_back(bacSi);
}

bool ChiaBacSi::PhanBoBenhNhan(const Patient& benhNhan)
{
    BacSi* bacSiPhuHop = nullptr;
    string khoaBenhNhan = ChuanHoaTenKhoa(benhNhan.department);

    for (BacSi* bacSi : DanhSachBacSiTruc)
    {
        if (bacSi->TenKhoa != khoaBenhNhan) continue;

        if (bacSiPhuHop == nullptr || bacSi->SoBenhNhanDangCho < bacSiPhuHop->SoBenhNhanDangCho)
            bacSiPhuHop = bacSi;
    }

    if (bacSiPhuHop == nullptr)
    {
        cout << "Khong tim thay bac si phu hop cho khoa: " << benhNhan.department << endl;
        return false;
    }

    bacSiPhuHop->DanhSachCho.ThemBenhNhan(benhNhan);
    bacSiPhuHop->SoBenhNhanDangCho++;
    BangDinhTuyen[benhNhan.id] = bacSiPhuHop;

    return true;
}

bool ChiaBacSi::ThuyenChuyenBacSi(const string& maBacSiCu)
{
    BacSi* bacSiCu = TimBacSiTheoMa(maBacSiCu);

    if (bacSiCu == nullptr) {
        cout << "Khong tim thay bac si: " << maBacSiCu << endl;
        return false;
    }

    BacSi* bacSiMoi = nullptr;

    for (BacSi* bacSi : DanhSachBacSiTruc)
    {
        if (bacSi == bacSiCu) continue;
        if (bacSi->TenKhoa != bacSiCu->TenKhoa) continue;

        if (bacSiMoi == nullptr || bacSi->SoBenhNhanDangCho < bacSiMoi->SoBenhNhanDangCho)
            bacSiMoi = bacSi;
    }

    if (bacSiMoi == nullptr) {
        cout << "Khong co bac si cung khoa de thuyen chuyen." << endl;
        return false;
    }

    for (const Patient& benhNhan : bacSiCu->DanhSachCho.MangDuLieu)
    {
        bacSiMoi->DanhSachCho.ThemBenhNhan(benhNhan);
        BangDinhTuyen[benhNhan.id] = bacSiMoi;
    }

    bacSiMoi->SoBenhNhanDangCho += bacSiCu->SoBenhNhanDangCho;
    bacSiCu->DanhSachCho.LamSach();
    bacSiCu->SoBenhNhanDangCho = 0;

    return true;
}

bool ChiaBacSi::KhamXongVaChuyenBuocCuoi(const string& idBenhNhan, FixedCapacityList& danhSachDaKham)
{
    BacSi* bacSi = TimBacSiCuaBenhNhan(idBenhNhan);

    if (bacSi == nullptr) {
        cout << "Khong tim thay benh nhan: " << idBenhNhan << endl;
        return false;
    }

    if (bacSi->DanhSachCho.MangDuLieu.empty()) {
        cout << "Hang doi cua bac si dang rong." << endl;
        return false;
    }

    if (bacSi->DanhSachCho.MangDuLieu[0].id != idBenhNhan) {
        cout << "Benh nhan " << idBenhNhan << " chua den luot kham." << endl;
        return false;
    }

    Patient benhNhan;
    if (!bacSi->DanhSachCho.LayBenhNhanUuTienNhat(benhNhan)) return false;

    bacSi->SoBenhNhanDangCho--;
    BangDinhTuyen.erase(idBenhNhan);

    insertExaminedPatient(danhSachDaKham, benhNhan);

    return true;
}

bool ChiaBacSi::KhamXongVaChuyenBuocCuoi(const string& idBenhNhan, const string& chanDoan, const checkoutTime& thoiGian, FixedCapacityList& danhSachDaKham)
{
    BacSi* bacSi = TimBacSiCuaBenhNhan(idBenhNhan);

    if (bacSi == nullptr) {
        cout << "Khong tim thay benh nhan: " << idBenhNhan << endl;
        return false;
    }

    if (bacSi->DanhSachCho.MangDuLieu.empty()) {
        cout << "Hang doi cua bac si dang rong." << endl;
        return false;
    }

    if (bacSi->DanhSachCho.MangDuLieu[0].id != idBenhNhan) {
        cout << "Benh nhan " << idBenhNhan << " chua den luot kham." << endl;
        return false;
    }

    Patient benhNhan;
    if (!bacSi->DanhSachCho.LayBenhNhanUuTienNhat(benhNhan)) return false;

    benhNhan.lastDiagnosis = chanDoan;
    benhNhan.time = thoiGian;

    bacSi->SoBenhNhanDangCho--;
    BangDinhTuyen.erase(idBenhNhan);

    insertExaminedPatient(danhSachDaKham, benhNhan);

    return true;
}

BacSi* ChiaBacSi::TimBacSiTheoMa(const string& maBacSi)
{
    for (BacSi* bacSi : DanhSachBacSiTruc)
        if (bacSi->MaBacSi == maBacSi) return bacSi;

    return nullptr;
}

BacSi* ChiaBacSi::TimBacSiCuaBenhNhan(const string& idBenhNhan)
{
    auto it = BangDinhTuyen.find(idBenhNhan);

    if (it == BangDinhTuyen.end()) return nullptr;

    return it->second;
}

void ChiaBacSi::InDanhSachBacSi() const
{
    cout << "\n===== DANH SACH BAC SI =====" << endl;

    for (const BacSi* bacSi : DanhSachBacSiTruc)
    {
        cout << bacSi->MaBacSi << " | " << bacSi->TenBacSi << " | " << bacSi->TenKhoa << " | Dang cho: " << bacSi->SoBenhNhanDangCho << endl;
    }
}