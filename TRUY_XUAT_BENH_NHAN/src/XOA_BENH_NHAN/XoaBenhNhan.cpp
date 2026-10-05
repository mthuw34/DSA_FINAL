#include "XoaBenhNhan.h"

bool XoaBenhNhan::xoaBenhNhan(int patientId) {
    //Điều kiện tiên quyết: ID bệnh nhân phải dương.
    return DBXoaBenhNhan::xoaBenhNhan(patientId);
}
