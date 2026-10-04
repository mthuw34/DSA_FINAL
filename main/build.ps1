param([string]$Compiler = "g++", [ValidatePattern('^[A-Za-z0-9_-]+\.exe$')][string]$OutputName = "hospital_web.exe")
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $PSScriptRoot "build"
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
$buildRoot = Join-Path $env:TEMP ("dsa-build-" + [guid]::NewGuid().ToString("N"))
$buildExitCode = 1
$linked = $false
$sources = @(
    "main/main.cpp", "main/WebService.cpp",
    "QUAN_LY_BENH_NHAN/src/CHECK_IN/khoa.cpp",
    "THAY_DOI_MUC_DO_UU_TIEN/src/DONG_BO/PrioritySync.cpp",
    "THAY_DOI_MUC_DO_UU_TIEN/src/THAY_DOI_TU_DONG/AutoPriority.cpp",
    "THAY_DOI_MUC_DO_UU_TIEN/src/THAY_DOI_TU_DONG/Functions_AutoPriority.cpp",
    "THAY_DOI_MUC_DO_UU_TIEN/src/THAY_DOI_TU_DONG/WorkingTime.cpp",
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
foreach ($directory in $includeDirectories) { $arguments += "-I" + $directory }
foreach ($source in $sources) { $arguments += $source }
$arguments += @("-o", "main/build/$OutputName", "-lsqlite3", "-lws2_32", "-lmswsock", "-pthread")
try {
    New-Item -ItemType Junction -Path $buildRoot -Target $repoRoot | Out-Null
    $linked = $true
    Push-Location $buildRoot
    try {
        & $Compiler @arguments
        $buildExitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }
}
finally {
    if ($linked) {
        $junction = Get-Item -LiteralPath $buildRoot -Force
        if (($junction.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            & $env:ComSpec /d /c ('rmdir "' + $buildRoot + '"')
            if ($LASTEXITCODE -ne 0) { throw "Khong go duoc junction tam: $buildRoot" }
        }
    }
}
if ($buildExitCode -ne 0) { throw "Build web that bai (exit $buildExitCode)" }
Write-Host "Build thanh cong: $buildDirectory\$OutputName"
