param(
    [string]$ToolsDir = (Join-Path (Split-Path $PSScriptRoot -Parent) '.tools')
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$downloadDir = Join-Path $ToolsDir 'downloads'
$w64Dir = Join-Path $ToolsDir 'w64devkit\w64devkit'
$vcpkgDir = Join-Path $ToolsDir 'vcpkg'
$mysqlDir = Join-Path $ToolsDir 'mysql'
$mysqlDataDir = Join-Path $ToolsDir 'mysql-data'

New-Item -ItemType Directory -Force -Path $downloadDir | Out-Null

function Download-File {
    param(
        [string]$Url,
        [string]$Destination
    )

    Write-Host "Downloading $Url"
    $partial = "$Destination.part"
    if (Test-Path $partial) { Remove-Item -LiteralPath $partial -Force }
    & curl.exe -L --fail --retry 3 --ssl-no-revoke --output $partial $Url
    if ($LASTEXITCODE -ne 0) {
        throw "Download failed: $Url"
    }
    Move-Item -LiteralPath $partial -Destination $Destination -Force
}

$env:PATH = "$w64Dir\bin;$vcpkgDir;$env:PATH"

# w64devkit provides a modern g++ for the C++17 toolchain.
$w64Archive = Join-Path $downloadDir 'w64devkit-x64-2.10.0.7z.exe'
if (-not (Test-Path (Join-Path $w64Dir 'bin\g++.exe'))) {
    Download-File 'https://github.com/skeeto/w64devkit/releases/download/v2.10.0/w64devkit-x64-2.10.0.7z.exe' $w64Archive
    New-Item -ItemType Directory -Force -Path $w64Dir | Out-Null
    & $w64Archive "-o$w64Dir" -y
}

# vcpkg and project libraries.
if (-not (Test-Path (Join-Path $vcpkgDir 'vcpkg.exe'))) {
    & git clone --depth 1 https://github.com/microsoft/vcpkg.git $vcpkgDir
    & (Join-Path $vcpkgDir 'bootstrap-vcpkg.bat') -disableMetrics
}

# libsodium 提供 Argon2id 口令哈希；gtest 与 yaml-cpp 分别供单元测试、配置解析使用。
& (Join-Path $vcpkgDir 'vcpkg.exe') install `
    cpp-httplib:x64-mingw-static `
    nlohmann-json:x64-mingw-static `
    libsodium:x64-mingw-static `
    libmariadb:x64-mingw-static `
    yaml-cpp:x64-mingw-static `
    gtest:x64-mingw-static

# MySQL Community Server no-install archive.
$mysqlArchive = Join-Path $downloadDir 'mysql-8.0.46-winx64.zip'
if (-not (Test-Path (Join-Path $mysqlDir 'bin\mysqld.exe'))) {
    Download-File 'https://dev.mysql.com/get/Downloads/MySQL-8.0/mysql-8.0.46-winx64.zip' $mysqlArchive
    $mysqlExtract = Join-Path $ToolsDir 'mysql-extract'
    if (Test-Path $mysqlExtract) { Remove-Item -LiteralPath $mysqlExtract -Recurse -Force }
    Expand-Archive -LiteralPath $mysqlArchive -DestinationPath $mysqlExtract
    $inner = Get-ChildItem -LiteralPath $mysqlExtract -Directory | Select-Object -First 1
    Move-Item -LiteralPath $inner.FullName -Destination $mysqlDir
    Remove-Item -LiteralPath $mysqlExtract -Recurse -Force
}

$mysqlIni = Join-Path $mysqlDir 'my.ini'
if (-not (Test-Path $mysqlIni)) {
    $content = @"
[mysqld]
basedir=$mysqlDir
datadir=$mysqlDataDir
port=3306
character-set-server=utf8mb4
collation-server=utf8mb4_unicode_ci

[client]
port=3306
"@
    Set-Content -LiteralPath $mysqlIni -Value $content -Encoding ASCII
}

if (-not (Test-Path $mysqlDataDir)) {
    & (Join-Path $mysqlDir 'bin\mysqld.exe') --initialize-insecure --console "--basedir=$mysqlDir" "--datadir=$mysqlDataDir"
}

Write-Host ''
Write-Host 'Dependencies installed under:' $ToolsDir
Write-Host 'Toolchain:' (Join-Path $w64Dir 'bin\g++.exe')
Write-Host 'CMake:' (Join-Path $w64Dir 'bin\cmake.exe')
Write-Host 'Ninja:' (Join-Path $w64Dir 'bin\ninja.exe')
Write-Host 'vcpkg:' (Join-Path $vcpkgDir 'vcpkg.exe')
Write-Host 'MySQL:' (Join-Path $mysqlDir 'bin\mysqld.exe')
