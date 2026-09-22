#ifndef BANG_CHECK_IN_H
#define BANG_CHECK_IN_H

#include <sqlite3.h>

void hienThiBangCheckIn(sqlite3* db);

bool xoaToanBoCheckIn(
    sqlite3* db
);

bool xoaMotCheckIn(
    sqlite3* db,
    int checkinId
);

#endif