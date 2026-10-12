$ErrorActionPreference = 'Stop'

$root = Split-Path $PSScriptRoot -Parent
$tools = Join-Path $root '.tools'
$build = Join-Path $root 'build'
$mysqlBin = Join-Path $tools 'mysql\bin'

$env:PATH = (Join-Path $tools 'w64devkit\w64devkit\bin') + ';' + $env:PATH

function Test-TcpPort {
    param([int]$Port)

    $client = New-Object System.Net.Sockets.TcpClient
    try {
        $client.Connect('127.0.0.1', $Port)
        return $true
    } catch {
        return $false
    } finally {
        $client.Dispose()
    }
}

function Wait-ForTcpPort {
    param(
        [int]$Port,
        [int]$TimeoutSeconds
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        if (Test-TcpPort -Port $Port) {
            return $true
        }
        Start-Sleep -Milliseconds 500
    }
    return $false
}

if (-not (Get-Process -Name mysqld -ErrorAction SilentlyContinue)) {
    Start-Process `
        -FilePath (Join-Path $mysqlBin 'mysqld.exe') `
        -ArgumentList @(
            "--defaults-file=$(Join-Path $tools 'mysql\my.ini')",
            '--console'
        ) `
        -RedirectStandardOutput (Join-Path $build 'mysql_console.log') `
        -RedirectStandardError (Join-Path $build 'mysql_error.log') `
        -WindowStyle Hidden | Out-Null
}

if (-not (Wait-ForTcpPort -Port 3306 -TimeoutSeconds 20)) {
    throw 'MySQL did not start on port 3306.'
}

if (-not (Get-Process -Name web_server -ErrorAction SilentlyContinue)) {
    Start-Process `
        -FilePath (Join-Path $build 'web_server.exe') `
        -WorkingDirectory $root `
        -RedirectStandardOutput (Join-Path $build 'web_server.log') `
        -RedirectStandardError (Join-Path $build 'web_server_error.log') `
        -WindowStyle Hidden | Out-Null
}

$deadline = (Get-Date).AddSeconds(20)
$healthStatus = ''
while ((Get-Date) -lt $deadline) {
    $healthStatus = & curl.exe -s -o NUL -w '%{http_code}' `
        'http://127.0.0.1:8080/api/health'
    if ($healthStatus -eq '200') {
        break
    }
    Start-Sleep -Milliseconds 500
}

if ($healthStatus -ne '200') {
    throw 'web_server did not become healthy on port 8080.'
}

if (-not (Get-Process -Name judge_worker -ErrorAction SilentlyContinue)) {
    Start-Process `
        -FilePath (Join-Path $build 'judge_worker.exe') `
        -WorkingDirectory $root `
        -RedirectStandardOutput (Join-Path $build 'judge_worker.log') `
        -RedirectStandardError (Join-Path $build 'judge_worker_error.log') `
        -WindowStyle Hidden | Out-Null
}

Start-Sleep -Seconds 1

Write-Host 'CodeHunter local services are running:'
Write-Host '  MySQL        : 127.0.0.1:3306'
Write-Host '  web_server   : http://127.0.0.1:8080'
Write-Host '  judge_worker : running'
Write-Host ''
Write-Host 'Open: http://127.0.0.1:8080/problems.html'
