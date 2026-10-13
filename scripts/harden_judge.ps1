# Windows 本地加固脚本（配合 SPEC 16.3 安全验收）
# 作用：为判题的固定执行路径添加防火墙规则，阻断用户代码的出网/入网流量。
# 用法（必须以管理员身份运行 PowerShell）：
#   powershell -ExecutionPolicy Bypass -File scripts\harden_judge.ps1
#   powershell -ExecutionPolicy Bypass -File scripts\harden_judge.ps1 -Status
#   powershell -ExecutionPolicy Bypass -File scripts\harden_judge.ps1 -Remove

param(
    [switch]$Remove,
    [switch]$Status
)

$ErrorActionPreference = 'Stop'

$root = Split-Path $PSScriptRoot -Parent
$runner = Join-Path $root 'build\judge_tmp\runner\program.exe'
$ruleName = 'CodeHunter Judge Runner'

function Test-Administrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Show-Rules {
    foreach ($direction in @('out', 'in')) {
        $name = "$ruleName ($direction)"
        Write-Host "----- $name -----"
        & netsh advfirewall firewall show rule name="$name" | Out-Host
    }
}

if ($Status) {
    Write-Host '只读检查：仅显示当前防火墙规则，不会做任何修改。'
    Write-Host ''
    Show-Rules
    exit 0
}

if (-not (Test-Administrator)) {
    Write-Host '需要管理员权限。请以管理员身份打开 PowerShell 后重新运行：' -ForegroundColor Yellow
    Write-Host "  powershell -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -ForegroundColor Yellow
    exit 1
}

if ($Remove) {
    foreach ($direction in @('out', 'in')) {
        & netsh advfirewall firewall delete rule `
            name="$ruleName ($direction)" | Out-Null
    }
    Write-Host '已删除判题防火墙规则。'
    Show-Rules
    exit 0
}

# 判题执行目录（运行时由判题服务创建）
$runnerDir = Split-Path $runner -Parent
if (-not (Test-Path $runnerDir)) {
    New-Item -ItemType Directory -Path $runnerDir -Force | Out-Null
}

Write-Host '即将添加以下拦截规则（出站 + 入站），目标程序路径：'
Write-Host "  $runner"
Write-Host ''

foreach ($direction in @('out', 'in')) {
    $name = "$ruleName ($direction)"
    & netsh advfirewall firewall delete rule name="$name" | Out-Null
    & netsh advfirewall firewall add rule `
        name="$name" `
        dir=$direction `
        action=block `
        program="$runner" `
        enable=yes `
        profile=any | Out-Null

    if ($LASTEXITCODE -ne 0) {
        throw "添加防火墙规则失败：$name"
    }
}

Write-Host '加固完成：用户代码将无法通过该路径访问网络。'
Write-Host "如需撤销：powershell -ExecutionPolicy Bypass -File `"$PSCommandPath`" -Remove"
Write-Host ''
Write-Host '当前规则：'
Show-Rules
