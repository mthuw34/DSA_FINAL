#ifndef CHECK_IN_H
#define CHECK_IN_H

#include <sqlite3.h>

// true  = tiep tuc check-in
// false = nguoi dung nhap 0 de thoat
bool checkInMotBenhNhan(sqlite3* db);

#endif

