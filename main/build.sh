#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p main/build
g++ -std=c++17 -O1 -DASIO_STANDALONE -pthread \
    -Iinclude -ISAP_XEP_BAC_SI/src/THUAT_TOAN_CHINH \
    -ISAP_XEP_BAC_SI/src/QUAN_LY_THONG_TIN -ISAP_XEP_BAC_SI/src/THOI_GIAN \
    -ISAP_XEP_BAC_SI/src/XU_LY_DATA_BASE_neu_can \
    main/main.cpp main/WebService.cpp \
    QUAN_LY_BENH_NHAN/src/CHECK_IN/khoa.cpp \
    THAY_DOI_MUC_DO_UU_TIEN/src/DONG_BO/PrioritySync.cpp \
    THAY_DOI_MUC_DO_UU_TIEN/src/CAP_NHAT_THU_CONG/PriorityManager.cpp \
    TRUY_XUAT_BENH_NHAN/src/TAO_BANG/DBTaoBang.cpp \
    TRUY_XUAT_BENH_NHAN/src/TRUY_XUAT/DBTruyXuat.cpp \
    TRUY_XUAT_BENH_NHAN/src/TRUY_XUAT/QuanLyHangDoi.cpp \
    TRUY_XUAT_BENH_NHAN/src/TRUY_XUAT/ThuatToanSapXep.cpp \
    SAP_XEP_BAC_SI/src/THUAT_TOAN_CHINH/QuanLyKhamBenh.cpp \
    SAP_XEP_BAC_SI/src/XU_LY_DATA_BASE_neu_can/XuLyDatabase.cpp \
    SAP_XEP_BAC_SI/src/QUAN_LY_THONG_TIN/QuanLyBacSi.cpp \
    SAP_XEP_BAC_SI/src/QUAN_LY_THONG_TIN/BacSi.cpp \
    SAP_XEP_BAC_SI/src/THOI_GIAN/ThoiGian.cpp \
    DANG_KHAM/src/DatabaseDangKham.cpp \
    -o main/build/hospital_web -lsqlite3
