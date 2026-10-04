param([string]$Compiler = "g++", [ValidatePattern('^[A-Za-z0-9_-]+\.exe$')][string]$OutputName = "hospital_web.exe")
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $PSScriptRoot "build"
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
$sources = @(
    "main/main.cpp", "main/WebService.cpp",
    "QUAN_LY_BENH_NHAN/src/CHECK_IN/khoa.cpp",
    "THAY_DOI_MUC_DO_UU_TIEN/src/DONG_BO/PrioritySync.cpp",
    "THAY_DOI_MUC_DO_UU_TIEN/src/CAP_NHAT_THU_CONG/PriorityManager.cpp",
    "TRUY_XUAT_BENH_NHAN/src/TAO_BANG/DBTaoBang.cpp",
    "TRUY_XUAT_BENH_NHAN/src/TRUY_XUAT/DBTruyXuat.cpp",
    "TRUY_XUAT_BENH_NHAN/src/TRUY_XUAT/QuanLyHangDoi.cpp",
    "TRUY_XUAT_BENH_NHAN/src/TRUY_XUAT/ThuatToanSapXep.cpp",
    "SAP_XEP_BAC_SI/src/THUAT_TOAN_CHINH/QuanLyKhamBenh.cpp",
    "SAP_XEP_BAC_SI/src/XU_LY_DATA_BASE_neu_can/XuLyDatabase.cpp",
    "SAP_XEP_BAC_SI/src/QUAN_LY_THONG_TIN/QuanLyBacSi.cpp",
    "SAP_XEP_BAC_SI/src/QUAN_LY_THONG_TIN/BacSi.cpp",
    "SAP_XEP_BAC_SI/src/THOI_GIAN/ThoiGian.cpp",
    "DANG_KHAM/src/DatabaseDangKham.cpp"
)
$includeDirectories = @("include", "SAP_XEP_BAC_SI/src/THUAT_TOAN_CHINH",
    "SAP_XEP_BAC_SI/src/QUAN_LY_THONG_TIN", "SAP_XEP_BAC_SI/src/THOI_GIAN",
    "SAP_XEP_BAC_SI/src/XU_LY_DATA_BASE_neu_can")
$arguments = @("-std=c++17", "-O0", "-Wall", "-Wextra", "-DASIO_STANDALONE", "-D_WIN32_WINNT=0x0601")
foreach ($directory in $includeDirectories) { $arguments += "-I" + (Join-Path $repoRoot $directory) }
foreach ($source in $sources) { $arguments += Join-Path $repoRoot $source }
$arguments += @("-o", (Join-Path $buildDirectory $OutputName), "-lsqlite3", "-lws2_32", "-lmswsock", "-pthread")
& $Compiler @arguments
if ($LASTEXITCODE -ne 0) { throw "Build web that bai (exit $LASTEXITCODE)" }
Write-Host "Build thanh cong: $buildDirectory\$OutputName"
