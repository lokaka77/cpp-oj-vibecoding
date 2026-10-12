$root = Split-Path $PSScriptRoot -Parent
$mysqlAdmin = Join-Path $root '.tools\mysql\bin\mysqladmin.exe'

Get-Process -Name web_server, judge_worker -ErrorAction SilentlyContinue |
    Stop-Process -Force

if (Get-Process -Name mysqld -ErrorAction SilentlyContinue) {
    & $mysqlAdmin -u root shutdown
}

$deadline = (Get-Date).AddSeconds(20)
while ((Get-Date) -lt $deadline -and
       (Get-Process -Name mysqld -ErrorAction SilentlyContinue)) {
    Start-Sleep -Milliseconds 500
}

$remaining = Get-Process -Name mysqld, web_server, judge_worker `
    -ErrorAction SilentlyContinue

if ($remaining) {
    Write-Host 'Some processes are still running:'
    $remaining | Select-Object Id, ProcessName
} else {
    Write-Host 'CodeHunter local services stopped.'
}
