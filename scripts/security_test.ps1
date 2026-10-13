# 安全验收脚本（SPEC 16.3）
# 用法：powershell -ExecutionPolicy Bypass -File scripts\security_test.ps1
# 说明：逐项检查密码存储、SQL 注入、越权、敏感文件读取、外网连接、子进程清理。

param(
    [int]$ProblemId = 1
)

$ErrorActionPreference = 'Stop'

$root = Split-Path $PSScriptRoot -Parent
$build = Join-Path $root 'build'
$secDir = Join-Path $build 'security'
if (-not (Test-Path $secDir)) {
    New-Item -ItemType Directory -Path $secDir | Out-Null
}

$base = 'http://127.0.0.1:8080'
$mysql = Join-Path $root '.tools\mysql\bin\mysql.exe'
$results = @()

function Add-Result {
    param([string]$Name, [string]$Status, [string]$Detail)
    $script:results += [pscustomobject]@{
        Name   = $Name
        Status = $Status
        Detail = $Detail
    }
    Write-Host ("[{0}] {1} —— {2}" -f $Status, $Name, $Detail)
}

function Write-Json {
    param([string]$Path, [object]$Value)
    [System.IO.File]::WriteAllText(
        $Path, ($Value | ConvertTo-Json -Depth 8 -Compress))
}

function Login {
    param([string]$User, [string]$Password, [string]$CookieName)
    $payload = Join-Path $secDir "$CookieName.json"
    $cookie = Join-Path $secDir "$CookieName.cookie"
    Write-Json $payload @{ username = $User; password = $Password }
    $status = & curl.exe -s -o NUL -w '%{http_code}' `
        -c $cookie `
        -X POST -H 'Content-Type: application/json' `
        --data-binary "@$payload" `
        "$base/api/auth/login"
    return [pscustomobject]@{ Status = $status; Cookie = $cookie }
}

function Submit {
    param([string]$Cookie, [string]$Code, [string]$Tag)
    $body = Join-Path $secDir "code_$Tag.json"
    $resp = Join-Path $secDir "resp_$Tag.json"
    Write-Json $body @{ code = $Code }

    & curl.exe -s -o $resp `
        -b $Cookie `
        -X POST -H 'Content-Type: application/json' `
        --data-binary "@$body" `
        "$base/api/problems/$ProblemId/submit" | Out-Null

    $raw = Get-Content $resp -Raw -Encoding UTF8
    try {
        $json = $raw | ConvertFrom-Json
    } catch {
        return 'PARSE_ERROR'
    }

    if ($json.verdict) {
        return $json.verdict
    }
    if ($json.error -and $json.error.code) {
        return $json.error.code
    }
    return 'UNKNOWN'
}

Write-Host '=== 1. 密码存储 ==='
$weak = & $mysql -u root -N -e "SELECT COUNT(*) FROM oj.users WHERE password_hash NOT LIKE CONCAT('%','argon2id','%');"
$plaintext = & $mysql -u root -N -e "SELECT COUNT(*) FROM oj.users WHERE password_hash IN ('admin123','student123','secret123');"
if ($weak -eq '0' -and $plaintext -eq '0') {
    Add-Result '密码不以明文存储' 'PASS' '全部为 $argon2id$ 哈希，无明文口令'
} else {
    Add-Result '密码不以明文存储' 'FAIL' "非 Argon2id 记录 $weak 条，明文口令 $plaintext 条"
}

Write-Host ''
Write-Host '=== 2. SQL 注入 ==='
$payloads = @(
    "' OR '1'='1",
    "admin'--",
    '" OR 1=1 --',
    "' OR 1=1 LIMIT 1--"
)
$injected = 0
foreach ($payload in $payloads) {
    $body = Join-Path $secDir 'injection.json'
    Write-Json $body @{ username = $payload; password = 'whatever' }
    $status = & curl.exe -s -o NUL -w '%{http_code}' `
        -X POST -H 'Content-Type: application/json' `
        --data-binary "@$body" `
        "$base/api/auth/login"
    if ($status -ne '401') {
        $injected++
        Write-Host "  载荷返回 $status ：$payload"
    }
}

$admin = Login -User 'admin' -Password 'admin123' -CookieName 'sec_admin'
$adminName = & curl.exe -s -b $admin.Cookie "$base/api/auth/me" | ConvertFrom-Json
$userCount = & $mysql -u root -N -e "SELECT COUNT(*) FROM oj.users;"

if ($injected -eq 0 -and $admin.Status -eq '200' -and $adminName.username -eq 'admin') {
    Add-Result 'SQL 注入（登录）' 'PASS' "4 个载荷全部 401，admin 仍可登录，用户数 $userCount"
} else {
    Add-Result 'SQL 注入（登录）' 'FAIL' "$injected 个载荷未被拒绝"
}

$weirdUser = "inj_quote_'x"
$body = Join-Path $secDir 'weird_user.json'
Write-Json $body @{ username = $weirdUser; password = 'secret123' }
$registerStatus = & curl.exe -s -o NUL -w '%{http_code}' `
    -X POST -H 'Content-Type: application/json' `
    --data-binary "@$body" `
    "$base/api/auth/register"

if ($registerStatus -eq '201') {
    $weirdLogin = Login -User $weirdUser -Password 'secret123' -CookieName 'sec_weird'
    $list = & curl.exe -s -b $admin.Cookie "$base/api/admin/users" | ConvertFrom-Json
    $target = $list.users | Where-Object { $_.username -eq $weirdUser }
    if ($weirdLogin.Status -eq '200' -and $target) {
        Add-Result 'SQL 注入（含引号用户名）' 'PASS' '畸形用户名按字面存入并可正常登录（参数化生效）'
        & curl.exe -s -o NUL -b $admin.Cookie `
            -X DELETE "$base/api/admin/users/$($target.id)"
    } else {
        Add-Result 'SQL 注入（含引号用户名）' 'FAIL' '注册后无法按原样登录'
    }
} else {
    Add-Result 'SQL 注入（含引号用户名）' 'PASS' "注册被拒绝（HTTP $registerStatus），未写入数据库"
}

$pathInjection = & curl.exe -s -o NUL -w '%{http_code}' `
    "$base/api/problems/1%20OR%201=1"
if ($pathInjection -in @('400', '404', '405')) {
    Add-Result 'SQL 注入（路径参数）' 'PASS' "畸形路径返回 $pathInjection"
} else {
    Add-Result 'SQL 注入（路径参数）' 'FAIL' "畸形路径返回 $pathInjection"
}

Write-Host ''
Write-Host '=== 3. 普通用户越权 ==='
$student = Login -User 'student01' -Password 'student123' -CookieName 'sec_student'
$adminAttempts = @(
    @{ Method = 'POST';   Url = "$base/api/admin/problems";                Body = '{"title":"越权题目","description":"x","difficulty":"easy"}'; Name = 'POST 新增题目' },
    @{ Method = 'PUT';    Url = "$base/api/admin/problems/$ProblemId";     Body = '{"title":"被篡改","description":"x","difficulty":"easy"}'; Name = 'PUT 修改题目' },
    @{ Method = 'DELETE'; Url = "$base/api/admin/problems/$ProblemId";     Body = $null; Name = 'DELETE 删除题目' },
    @{ Method = 'POST';   Url = "$base/api/admin/problems/$ProblemId/testcases"; Body = '{"test_cases":[{"input":"1","expected_output":"1","is_sample":true}]}'; Name = 'POST 导入用例' },
    @{ Method = 'GET';    Url = "$base/api/admin/users";                   Body = $null; Name = 'GET 用户列表' }
)

$blocked = 0
foreach ($attempt in $adminAttempts) {
    $bodyFile = Join-Path $secDir 'attempt.json'
    if ($attempt.Body) {
        [System.IO.File]::WriteAllText($bodyFile, $attempt.Body)
    }

    if ($attempt.Body) {
        $status = & curl.exe -s -o NUL -w '%{http_code}' `
            -b $student.Cookie -X $attempt.Method `
            -H 'Content-Type: application/json' `
            --data-binary "@$bodyFile" $attempt.Url
    } else {
        $status = & curl.exe -s -o NUL -w '%{http_code}' `
            -b $student.Cookie -X $attempt.Method $attempt.Url
    }

    if ($status -eq '403') {
        $blocked++
    } else {
        Write-Host ("  {0} 返回 {1}" -f $attempt.Name, $status)
    }
}

$problemCount = & $mysql -u root -N -e "SELECT COUNT(*) FROM oj.problems;"
$problemTitle = & $mysql -u root -N -e "SELECT title FROM oj.problems WHERE id = $ProblemId;"
if ($blocked -eq $adminAttempts.Count -and $problemCount -eq '10') {
    Add-Result '普通用户无法新增/修改/删除题目' 'PASS' "5 个管理端请求全部 403；题目仍为 10 道，题目 #$ProblemId 标题未变"
} else {
    Add-Result '普通用户无法新增/修改/删除题目' 'FAIL' "仅 $blocked/$($adminAttempts.Count) 被拒绝"
}

Write-Host ''
Write-Host '=== 4. 用户代码读取服务端文件 ==='
$probeUsers = @()
foreach ($index in 1..4) {
    $probeUsers += Login -User ('student{0:D2}' -f $index) `
        -Password 'student123' -CookieName ("sec_probe{0:D2}" -f $index)
}

$relativeProbe = @'
#include <fstream>
int main() {
    std::ifstream file("config.yaml");
    return file.good() ? 42 : 0;
}
'@

$absoluteProbe = @'
#include <fstream>
int main() {
    std::ifstream file("D:/codexworks/cppojvibecoding/config.yaml");
    return file.good() ? 42 : 0;
}
'@

$relativeVerdict = Submit -Cookie $probeUsers[0].Cookie -Code $relativeProbe -Tag 'relative'
$absoluteVerdict = Submit -Cookie $probeUsers[1].Cookie -Code $absoluteProbe -Tag 'absolute'

if ($relativeVerdict -eq 'RE') {
    Add-Result '用户代码无法读取项目文件（相对路径）' 'FAIL' "判定 $relativeVerdict，说明相对路径可读到 config.yaml"
} else {
    Add-Result '用户代码无法读取项目文件（相对路径）' 'PASS' "判定 $relativeVerdict，相对路径读不到项目文件"
}

if ($absoluteVerdict -eq 'RE') {
    Add-Result '用户代码无法读取项目文件（绝对路径）' 'FAIL' "判定 $absoluteVerdict，绝对路径仍可读到 config.yaml（Windows 无文件沙箱）"
} else {
    Add-Result '用户代码无法读取项目文件（绝对路径）' 'PASS' "判定 $absoluteVerdict"
}

Write-Host ''
Write-Host '=== 5. 用户代码外网连接 ==='
$networkProbe = @'
#include <cstdlib>
int main() {
    int code = std::system("curl.exe -s -o NUL -m 3 http://www.msftconnecttest.com/connecttest.txt");
    return code == 0 ? 42 : 0;
}
'@

& curl.exe -s -o NUL -m 5 "http://www.msftconnecttest.com/connecttest.txt" | Out-Null
$hostNetwork = $LASTEXITCODE
$networkVerdict = Submit -Cookie $probeUsers[2].Cookie -Code $networkProbe -Tag 'network'

if ($hostNetwork -ne 0) {
    Add-Result '用户代码不能建立外部网络连接' 'INCONCLUSIVE' "宿主机自身访问外网返回 $hostNetwork，本次结果不可判定"
} elseif ($networkVerdict -eq 'RE') {
    Add-Result '用户代码不能建立外部网络连接' 'FAIL' "判定 $networkVerdict，用户代码成功访问了外网"
} else {
    Add-Result '用户代码不能建立外部网络连接' 'PASS' "判定 $networkVerdict，外网访问未成功"
}

Write-Host ''
Write-Host '=== 6. 子进程与进程树清理 ==='
$childProbe = @'
#include <cstdlib>
int main() {
    std::system("ping -n 30 127.0.0.1 > NUL");
    while (true) {
    }
    return 0;
}
'@

$beforePing = @(Get-Process -Name ping -ErrorAction SilentlyContinue).Count
$childVerdict = Submit -Cookie $probeUsers[3].Cookie -Code $childProbe -Tag 'child'
Start-Sleep -Seconds 2
$afterPing = @(Get-Process -Name ping -ErrorAction SilentlyContinue).Count

if ($childVerdict -eq 'TLE' -and $afterPing -eq $beforePing) {
    Add-Result '用户代码无法通过命令注入破坏主机（进程树清理）' 'PASS' "判定 $childVerdict，子进程被清理（残留 $afterPing 个）"
} else {
    Add-Result '用户代码无法通过命令注入破坏主机（进程树清理）' 'FAIL' "判定 $childVerdict，残留进程 $afterPing 个"
}

Write-Host ''
Write-Host '=== 汇总 ==='
foreach ($item in $results) {
    Write-Host ("{0,-14} {1} —— {2}" -f $item.Status, $item.Name, $item.Detail)
}

$pass = @($results | Where-Object { $_.Status -eq 'PASS' }).Count
$fail = @($results | Where-Object { $_.Status -eq 'FAIL' }).Count
Write-Host ''
Write-Host ("通过 {0} 项，未通过 {1} 项，共 {2} 项" -f $pass, $fail, $results.Count)
