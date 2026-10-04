FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ libsqlite3-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY include/ include/
COPY main/ main/
COPY QUAN_LY_BENH_NHAN/src/ QUAN_LY_BENH_NHAN/src/
COPY THAY_DOI_MUC_DO_UU_TIEN/src/ THAY_DOI_MUC_DO_UU_TIEN/src/
COPY TRUY_XUAT_BENH_NHAN/src/ TRUY_XUAT_BENH_NHAN/src/
COPY SAP_XEP_BAC_SI/src/ SAP_XEP_BAC_SI/src/
COPY DANG_KHAM/src/ DANG_KHAM/src/
RUN sed -i 's/\r$//' main/build.sh && sh main/build.sh

FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends libsqlite3-0 tzdata ca-certificates \
    && rm -rf /var/lib/apt/lists/*
ENV TZ=Asia/Ho_Chi_Minh HOSPITAL_BIND=0.0.0.0 PORT=18080
WORKDIR /app
COPY --from=build /app/main/build/hospital_web /usr/local/bin/hospital-web
COPY main/web/ main/web/
COPY SAP_XEP_BAC_SI/db/bac_si_500_chia_khoa.csv SAP_XEP_BAC_SI/db/bac_si_500_chia_khoa.csv
COPY main/deploy/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN sed -i 's/\r$//' /usr/local/bin/entrypoint.sh && chmod +x /usr/local/bin/entrypoint.sh \
    && mkdir -p /data SAP_XEP_BAC_SI/src QUAN_LY_BENH_NHAN THAY_DOI_MUC_DO_UU_TIEN TRUY_XUAT_BENH_NHAN DANG_KHAM
EXPOSE 18080
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
