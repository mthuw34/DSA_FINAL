#ifndef CHECK_IN_H
#define CHECK_IN_H

#include <sqlite3.h>

// true  = tiep tuc check-in
// false = nguoi dung nhap 0 hoac dau vao khong the doc tiep
bool checkInMotBenhNhan(sqlite3* db);

#endif

