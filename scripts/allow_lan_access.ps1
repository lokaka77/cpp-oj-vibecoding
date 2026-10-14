# 允许局域网（含手机）访问本机 Web 服务
# 用法（必须以管理员身份运行 PowerShell）：
#   powershell -ExecutionPolicy Bypass -File scripts\allow_lan_access.ps1            # 放行 8080
#   powershell -ExecutionPolicy Bypass -File scripts\allow_lan_access.ps1 -Status    # 查看规则
#   powershell -ExecutionPolicy Bypass -File scripts\allow_lan_access.ps1 -Remove    # 撤销放行

param(
    [switch]$Remove,
    [switch]$Status,
    [int]$Port = 8080
)

$ErrorActionPreference = 'Stop'

$ruleName = "CodeHunter Web Access (TCP $Port)"

function Test-Administrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Show-LanAddress {
    # 用 ipconfig 解析（Get-NetIPAddress 在部分受限环境下不可用）
    $addresses = @(
        ipconfig |
            Select-String -Pattern 'IPv4.*: (\d+\.\d+\.\d+\.\d+)' |
            ForEach-Object { $_.Matches[0].Groups[1].Value } |
            Where-Object { $_ -notlike '169.254.*' } |
            Select-Object -Unique
    )

    if ($addresses.Count -eq 0) {
        Write-Host '未找到局域网 IPv4 地址，请用 ipconfig 自查。'
        return
    }

    foreach ($address in $addresses) {
        Write-Host "  手机访问地址：http://${address}:$Port/"
    }
}

if ($Status) {
    Write-Host '只读检查：仅显示当前放行规则。'
    Write-Host ''
    & netsh advfirewall firewall show rule name="$ruleName"
    exit 0
}

if (-not (Test-Administrator)) {
    Write-Host '需要管理员权限。请以管理员身份打开 PowerShell 后重新运行：' -ForegroundColor Yellow
    Write-Host "  powershell -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -ForegroundColor Yellow
    exit 1
}

if ($Remove) {
    & netsh advfirewall firewall delete rule name="$ruleName" | Out-Null
    Write-Host "已撤销放行规则：$ruleName"
    exit 0
}

& netsh advfirewall firewall delete rule name="$ruleName" | Out-Null
& netsh advfirewall firewall add rule `
    name="$ruleName" `
    dir=in `
    action=allow `
    protocol=TCP `
    localport=$Port `
    profile=any | Out-Null

if ($LASTEXITCODE -ne 0) {
    throw '添加放行规则失败'
}

Write-Host "已放行 TCP $Port 入站（规则名：$ruleName）"
Write-Host ''
Write-Host '前提：手机与笔记本在同一局域网（同一 Wi-Fi），且服务已启动。'
Show-LanAddress
