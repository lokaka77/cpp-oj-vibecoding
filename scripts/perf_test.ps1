# 性能验收脚本（SPEC 16.2）
# 用法：powershell -ExecutionPolicy Bypass -File scripts\perf_test.ps1
# 说明：
#   1. 单用户基线：1 个用户提交 1 次，记录端到端耗时；
#   2. 20 用户并发：student01~student20 同时提交，记录 p50 / p95 / 最大值；
#   3. TLE 检查：提交死循环代码，核对结论与耗时；
#   4. 稳定性检查：连续多轮提交，核对服务存活与判题进程无残留。

param(
    [int]$ConcurrentUsers = 20,
    [int]$ProblemId = 1,
    [int]$StabilityRounds = 2
)

$ErrorActionPreference = 'Stop'

$root = Split-Path $PSScriptRoot -Parent
$build = Join-Path $root 'build'
$perfDir = Join-Path $build 'perf'
if (-not (Test-Path $perfDir)) {
    New-Item -ItemType Directory -Path $perfDir | Out-Null
}

$base = 'http://127.0.0.1:8080'
$password = 'student123'
$codeFile = Join-Path $perfDir 'code.json'
$tleFile = Join-Path $perfDir 'code_tle.json'

$correctCode = @'
#include <iostream>
int main() {
    long long a = 0;
    long long b = 0;
    std::cin >> a >> b;
    std::cout << a + b << std::endl;
    return 0;
}
'@

$tleCode = @'
#include <iostream>
int main() {
    while (true) {
    }
    return 0;
}
'@

[System.IO.File]::WriteAllText(
    $codeFile, (@{ code = $correctCode } | ConvertTo-Json -Compress))
[System.IO.File]::WriteAllText(
    $tleFile, (@{ code = $tleCode } | ConvertTo-Json -Compress))

function Get-Stats {
    param([double[]]$Values)

    $sorted = $Values | Sort-Object
    $count = $sorted.Count
    $mean = ($sorted | Measure-Object -Average).Average
    $p50 = $sorted[[math]::Floor(($count - 1) * 0.50)]
    $p95 = $sorted[[math]::Ceiling(($count - 1) * 0.95)]
    $max = $sorted[$count - 1]

    return [pscustomobject]@{
        Mean = [math]::Round($mean, 3)
        P50  = [math]::Round($p50, 3)
        P95  = [math]::Round($p95, 3)
        Max  = [math]::Round($max, 3)
    }
}

function Connect-Users {
    param([int]$Count)

    $users = @()
    for ($index = 1; $index -le $Count; $index++) {
        $name = 'student{0:D2}' -f $index
        $payload = Join-Path $perfDir "$name.json"
        $cookie = Join-Path $perfDir "$name.cookie"

        [System.IO.File]::WriteAllText(
            $payload,
            (@{ username = $name; password = $password } |
                ConvertTo-Json -Compress))

        $status = & curl.exe -s -o NUL -w '%{http_code}' `
            -c $cookie `
            -X POST -H 'Content-Type: application/json' `
            --data-binary "@$payload" `
            "$base/api/auth/login"

        if ($status -ne '200') {
            throw "登录失败：$name（HTTP $status）"
        }

        $users += [pscustomobject]@{ User = $name; Cookie = $cookie }
    }

    return $users
}

function Invoke-ParallelSubmissions {
    param(
        [object[]]$Users,
        [string]$BodyFile,
        [string]$Tag
    )

    $pending = @()
    $wallStart = Get-Date

    foreach ($user in $Users) {
        $timeFile = Join-Path $perfDir "time_${Tag}_$($user.User).txt"
        $respFile = Join-Path $perfDir "resp_${Tag}_$($user.User).json"

        if (Test-Path $timeFile) {
            Remove-Item -LiteralPath $timeFile -Force
        }
        if (Test-Path $respFile) {
            Remove-Item -LiteralPath $respFile -Force
        }

        # 手工拼命令行：带空格的参数必须自带引号，
        # 否则 Start-Process 会按空格拆散 --data-binary / -H 等参数。
        $url = "$base/api/problems/$ProblemId/submit"
        $argumentLine =
            '-s -b "' + $user.Cookie + '"' +
            ' -X POST' +
            ' -H "Content-Type: application/json"' +
            ' --data-binary "@' + $BodyFile + '"' +
            ' -o "' + $respFile + '"' +
            ' -w "%{time_total}"' +
            ' "' + $url + '"'

        $process = Start-Process -FilePath 'curl.exe' `
            -ArgumentList $argumentLine `
            -PassThru `
            -WindowStyle Hidden `
            -RedirectStandardOutput $timeFile

        $pending += [pscustomobject]@{
            User  = $user.User
            Proc  = $process
            Time  = $timeFile
            Resp  = $respFile
        }
    }

    foreach ($item in $pending) {
        $item.Proc.WaitForExit() | Out-Null
    }

    $wallSeconds = ((Get-Date) - $wallStart).TotalSeconds

    $results = @()
    foreach ($item in $pending) {
        $timeText = ''
        if (Test-Path $item.Time) {
            $timeText = (Get-Content $item.Time -Raw).Trim()
        }

        $match = [regex]::Match($timeText, '[0-9]+(\.[0-9]+)?')
        if (-not $match.Success) {
            throw "无法解析耗时输出：$($item.User) -> '$timeText'"
        }
        $seconds = [double]$match.Value
        $verdict = 'UNKNOWN'
        if (Test-Path $item.Resp) {
            $raw = Get-Content $item.Resp -Raw -Encoding UTF8
            try {
                $verdict = ($raw | ConvertFrom-Json).verdict
            } catch {
                $verdict = 'PARSE_ERROR'
            }
        }

        $results += [pscustomobject]@{
            User     = $item.User
            Seconds  = $seconds
            Verdict  = $verdict
        }
    }

    return [pscustomobject]@{
        Results     = $results
        WallSeconds = [math]::Round($wallSeconds, 2)
    }
}

function Get-JudgeProcessCount {
    $names = @('g++', 'cc1plus', 'program')
    $total = 0
    foreach ($name in $names) {
        $found = @(Get-Process -Name $name -ErrorAction SilentlyContinue)
        $total += $found.Count
    }
    return $total
}

Write-Host '=== 1. 单用户基线 ==='
$single = Connect-Users -Count 1
$baseline = Invoke-ParallelSubmissions -Users $single -BodyFile $codeFile -Tag 'baseline'
$baselineResult = $baseline.Results[0]
Write-Host ("提交结论：{0}    端到端耗时：{1} 秒" -f `
    $baselineResult.Verdict, $baselineResult.Seconds)

Start-Sleep -Seconds 4

Write-Host ''
Write-Host "=== 2. $ConcurrentUsers 用户并发 ==="
$users = Connect-Users -Count $ConcurrentUsers
$burst = Invoke-ParallelSubmissions -Users $users -BodyFile $codeFile -Tag 'burst'
$stats = Get-Stats -Values ($burst.Results | ForEach-Object { $_.Seconds })
$accepted = @($burst.Results | Where-Object { $_.Verdict -eq 'AC' }).Count
$rejected = @($burst.Results | Where-Object { $_.Verdict -eq 'RATE_LIMITED' }).Count

Write-Host ("并发提交：{0} 次    全部完成墙钟：{1} 秒" -f `
    $burst.Results.Count, $burst.WallSeconds)
Write-Host ("端到端耗时：平均 {0} 秒 / p50 {1} 秒 / p95 {2} 秒 / 最大 {3} 秒" -f `
    $stats.Mean, $stats.P50, $stats.P95, $stats.Max)
Write-Host ("结论分布：AC {0}    限流 {1}" -f $accepted, $rejected)
if ($stats.P95 -le 0.5) {
    Write-Host '500ms 目标：达标'
} else {
    Write-Host '500ms 目标：未达标'
}

Start-Sleep -Seconds 4

Write-Host ''
Write-Host '=== 3. TLE 检查（死循环） ==='
$tleUser = Connect-Users -Count 1
$tle = Invoke-ParallelSubmissions -Users $tleUser -BodyFile $tleFile -Tag 'tle'
$tleResult = $tle.Results[0]
Write-Host ("结论：{0}    端到端耗时：{1} 秒" -f `
    $tleResult.Verdict, $tleResult.Seconds)

Start-Sleep -Seconds 4

Write-Host ''
Write-Host ("=== 4. 稳定性（{0} 轮 × {1} 用户） ===" -f `
    $StabilityRounds, $ConcurrentUsers)
$beforeCounts = @(
    (Get-Process -Name web_server -ErrorAction SilentlyContinue).Id,
    (Get-Process -Name judge_worker -ErrorAction SilentlyContinue).Id
)
$processLeakBefore = Get-JudgeProcessCount

$roundStats = @()
for ($round = 1; $round -le $StabilityRounds; $round++) {
    $result = Invoke-ParallelSubmissions -Users $users -BodyFile $codeFile -Tag "round$round"
    $roundStat = Get-Stats -Values ($result.Results | ForEach-Object { $_.Seconds })
    $roundAc = @($result.Results | Where-Object { $_.Verdict -eq 'AC' }).Count
    $roundStats += [pscustomobject]@{
        Round = $round
        P50   = $roundStat.P50
        Max   = $roundStat.Max
        AC    = $roundAc
        Wall  = $result.WallSeconds
    }
    Write-Host ("第 {0} 轮：AC {1}/{2}    p50 {3} 秒    max {4} 秒    墙钟 {5} 秒" -f `
        $round, $roundAc, $result.Results.Count, $roundStat.P50, `
        $roundStat.Max, $result.WallSeconds)
}

$processLeakAfter = Get-JudgeProcessCount
$afterCounts = @(
    (Get-Process -Name web_server -ErrorAction SilentlyContinue).Id,
    (Get-Process -Name judge_worker -ErrorAction SilentlyContinue).Id
)
$health = & curl.exe -s -o NUL -w '%{http_code}' "$base/api/health"

Write-Host ''
Write-Host '=== 汇总 ==='
Write-Host ("服务存活：health {0}    web_server {1}    judge_worker {2}" -f `
    $health, ($afterCounts[0] -join ','), ($afterCounts[1] -join ','))
Write-Host ("判题进程残留：提交前 {0} 个 → 提交后 {1} 个" -f `
    $processLeakBefore, $processLeakAfter)
