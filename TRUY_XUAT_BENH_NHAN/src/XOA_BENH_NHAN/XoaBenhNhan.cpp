#include "XoaBenhNhan.h"

bool XoaBenhNhan::xoaBenhNhan(int patientId) {
    // Precondition: patientId must be positive.
    return DBXoaBenhNhan::xoaBenhNhan(patientId);
}
