#!/bin/sh
set -eu
: "${HOSPITAL_PASSWORD:?Set HOSPITAL_PASSWORD before starting the Internet server}"
for module in QUAN_LY_BENH_NHAN THAY_DOI_MUC_DO_UU_TIEN TRUY_XUAT_BENH_NHAN DANG_KHAM; do
    mkdir -p "/data/$module"
    # Runtime image has no local databases. These links keep existing module paths compatible.
    if [ ! -e "/app/$module/db" ]; then
        ln -s "/data/$module" "/app/$module/db"
    fi
done
exec /usr/local/bin/hospital-web /app
