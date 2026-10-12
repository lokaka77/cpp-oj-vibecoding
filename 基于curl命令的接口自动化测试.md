# 基于 curl 命令的接口自动化测试

本文用于重复验证当前 OJ 后端接口是否与 [API.md](API.md) 保持一致。

## 1. 适用范围

覆盖以下接口：

- 健康检查
- 注册、登录、退出、当前用户
- 题目列表、题目详情
- 运行样例
- 提交判题
- 提交频率限制
- 普通用户与管理员权限边界
- 管理员题目增删改
- 管理员测试用例导入

## 2. 前置条件

- 在项目根目录执行：

```text
D:\codexworks\cppojvibecoding
```

- 已安装 MySQL、g++、cpp-httplib 等依赖。
- 已执行过 `scripts/init_db.sql`。
- 8080、3306 端口未被占用。
- 当前用户对 `build` 和 `.tools` 目录有读写权限。

## 3. 一键测试脚本

在项目根目录打开 PowerShell，执行下面脚本。

脚本会：

1. 启动 MySQL。
2. 启动 `web_server` 和 `judge_worker`。
3. 使用 curl 调用全部接口。
4. 校验状态码和关键字段。
5. 删除临时测试数据。
6. 关闭 Web 服务、judge worker 和 MySQL。

```powershell
$ErrorActionPreference = 'Stop'

$root = (Get-Location).Path
$tools = Join-Path $root '.tools'
$build = Join-Path $root 'build'
$mysqlBin = Join-Path $tools 'mysql\bin'
$mysqlIni = Join-Path $tools 'mysql\my.ini'

$env:PATH = (Join-Path $tools 'w64devkit\w64devkit\bin') + ';' + $env:PATH

$mysqlExe = Join-Path $mysqlBin 'mysqld.exe'
$mysqlAdmin = Join-Path $mysqlBin 'mysqladmin.exe'
$mysqlClient = Join-Path $mysqlBin 'mysql.exe'
$webServer = Join-Path $build 'web_server.exe'
$judgeWorker = Join-Path $build 'judge_worker.exe'

$webProcess = $null
$workerProcess = $null
$mysqlProcess = $null

$user = 'apiuser' + (Get-Date -Format 'HHmmss')
$userFile = Join-Path $build 'api_user.json'
$adminFile = Join-Path $build 'api_admin.json'
$codeFile = Join-Path $build 'api_code.json'
$userCookie = Join-Path $build 'api_user_cookie.txt'
$adminCookie = Join-Path $build 'api_admin_cookie.txt'
$responseFile = Join-Path $build 'api_response.json'

function Assert-Status {
    param(
        [string]$Name,
        [string]$Actual,
        [string]$Expected
    )

    if ($Actual -ne $Expected) {
        throw "$Name failed: expected $Expected, got $Actual"
    }

    Write-Host "PASS $Name [$Actual]"
}

function Write-JsonFile {
    param(
        [string]$Path,
        [object]$Value
    )

    $json = $Value | ConvertTo-Json -Depth 10 -Compress
    [System.IO.File]::WriteAllText($Path, $json)
}

try {
    $mysqlProcess = Start-Process `
        -FilePath $mysqlExe `
        -ArgumentList @("--defaults-file=$mysqlIni", '--console') `
        -RedirectStandardOutput (Join-Path $build 'mysql_console.log') `
        -RedirectStandardError (Join-Path $build 'mysql_error.log') `
        -WindowStyle Hidden `
        -PassThru

    Start-Sleep -Seconds 6

    $webProcess = Start-Process `
        -FilePath $webServer `
        -WorkingDirectory $root `
        -RedirectStandardOutput (Join-Path $build 'web_server.log') `
        -RedirectStandardError (Join-Path $build 'web_server_error.log') `
        -WindowStyle Hidden `
        -PassThru

    $workerProcess = Start-Process `
        -FilePath $judgeWorker `
        -WorkingDirectory $root `
        -RedirectStandardOutput (Join-Path $build 'judge_worker.log') `
        -RedirectStandardError (Join-Path $build 'judge_worker_error.log') `
        -WindowStyle Hidden `
        -PassThru

    Start-Sleep -Seconds 2

    $status = curl.exe -s -o $responseFile -w '%{http_code}' http://127.0.0.1:8080/api/health
    Assert-Status 'GET /api/health' $status '200'

    $status = curl.exe -s -o $responseFile -w '%{http_code}' http://127.0.0.1:8080/api/problems
    Assert-Status 'GET /api/problems public' $status '200'

    $status = curl.exe -s -o $responseFile -w '%{http_code}' http://127.0.0.1:8080/api/problems/1
    Assert-Status 'GET /api/problems/1 unauthenticated' $status '401'

    Write-JsonFile $userFile @{
        username = $user
        password = 'secret123'
    }

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -X POST -H 'Content-Type: application/json' `
        --data-binary "@$userFile" `
        http://127.0.0.1:8080/api/auth/register
    Assert-Status 'POST /api/auth/register' $status '201'

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -c $userCookie `
        -X POST -H 'Content-Type: application/json' `
        --data-binary "@$userFile" `
        http://127.0.0.1:8080/api/auth/login
    Assert-Status 'POST /api/auth/login user' $status '200'

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $userCookie `
        http://127.0.0.1:8080/api/auth/me
    Assert-Status 'GET /api/auth/me' $status '200'

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $userCookie `
        http://127.0.0.1:8080/api/problems/1
    Assert-Status 'GET /api/problems/1 user' $status '200'

    $code = @'
#include <iostream>
int main(){int a,b; std::cin>>a>>b; std::cout<<a+b;}
'@
    Write-JsonFile $codeFile @{ code = $code }

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $userCookie `
        -X POST -H 'Content-Type: application/json' `
        --data-binary "@$codeFile" `
        http://127.0.0.1:8080/api/problems/1/run
    Assert-Status 'POST /api/problems/1/run' $status '200'
    $runResult = Get-Content -Raw -Encoding UTF8 $responseFile | ConvertFrom-Json
    if (-not $runResult.passed) {
        throw 'run sample did not pass'
    }

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $userCookie `
        -X POST -H 'Content-Type: application/json' `
        --data-binary "@$codeFile" `
        http://127.0.0.1:8080/api/problems/1/submit
    Assert-Status 'POST /api/problems/1/submit first' $status '200'
    $submitResult = Get-Content -Raw -Encoding UTF8 $responseFile | ConvertFrom-Json
    if ($submitResult.verdict -ne 'AC') {
        throw "expected AC, got $($submitResult.verdict)"
    }

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $userCookie `
        -X POST -H 'Content-Type: application/json' `
        --data-binary "@$codeFile" `
        http://127.0.0.1:8080/api/problems/1/submit
    Assert-Status 'POST /api/problems/1/submit rate limited' $status '429'

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $userCookie `
        http://127.0.0.1:8080/api/admin/problems
    Assert-Status 'GET /api/admin/problems user forbidden' $status '403'

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $userCookie -c $userCookie `
        -X POST `
        http://127.0.0.1:8080/api/auth/logout
    Assert-Status 'POST /api/auth/logout' $status '200'

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $userCookie `
        http://127.0.0.1:8080/api/auth/me
    Assert-Status 'GET /api/auth/me after logout' $status '401'

    Write-JsonFile $adminFile @{
        username = 'admin'
        password = 'admin123'
    }

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -c $adminCookie `
        -X POST -H 'Content-Type: application/json' `
        --data-binary "@$adminFile" `
        http://127.0.0.1:8080/api/auth/login
    Assert-Status 'POST /api/auth/login admin' $status '200'

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $adminCookie `
        http://127.0.0.1:8080/api/admin/problems
    Assert-Status 'GET /api/admin/problems admin' $status '200'

    $problemTitle = 'API Test Problem ' + (Get-Date -Format 'HHmmss')
    $createFile = Join-Path $build 'api_create_problem.json'
    Write-JsonFile $createFile @{
        title = $problemTitle
        description = 'created by curl test'
    }

    curl.exe -s -o $responseFile `
        -b $adminCookie `
        -X POST -H 'Content-Type: application/json' `
        --data-binary "@$createFile" `
        http://127.0.0.1:8080/api/admin/problems
    $createResult = Get-Content -Raw -Encoding UTF8 $responseFile | ConvertFrom-Json
    if (-not $createResult.id) {
        throw 'admin problem create failed'
    }
    $problemId = $createResult.id
    Write-Host "PASS POST /api/admin/problems [id=$problemId]"

    $updateFile = Join-Path $build 'api_update_problem.json'
    Write-JsonFile $updateFile @{
        title = ($problemTitle + ' Updated')
        description = 'updated by curl test'
    }

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $adminCookie `
        -X PUT -H 'Content-Type: application/json' `
        --data-binary "@$updateFile" `
        "http://127.0.0.1:8080/api/admin/problems/$problemId"
    Assert-Status 'PUT /api/admin/problems/{id}' $status '200'

    $testCaseFile = Join-Path $build 'api_import_testcases.json'
    Write-JsonFile $testCaseFile @{
        test_cases = @(
            @{
                input = "1 2`n"
                expected_output = "3`n"
                is_sample = $true
            },
            @{
                input = "10 20`n"
                expected_output = "30`n"
                is_sample = $false
            }
        )
    }

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $adminCookie `
        -X POST -H 'Content-Type: application/json' `
        --data-binary "@$testCaseFile" `
        "http://127.0.0.1:8080/api/admin/problems/$problemId/testcases"
    Assert-Status 'POST /api/admin/problems/{id}/testcases' $status '200'

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $adminCookie `
        -X DELETE `
        "http://127.0.0.1:8080/api/admin/problems/$problemId"
    Assert-Status 'DELETE /api/admin/problems/{id}' $status '200'

    $status = curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $adminCookie `
        "http://127.0.0.1:8080/api/problems/$problemId"
    Assert-Status 'GET deleted problem' $status '404'

    & $mysqlClient -u root -e "DELETE FROM oj.users WHERE username='$user';"

    Write-Host ''
    Write-Host 'ALL API TESTS PASSED'
}
finally {
    if ($webProcess -and -not $webProcess.HasExited) {
        Stop-Process -Id $webProcess.Id -Force
    }
    if ($workerProcess -and -not $workerProcess.HasExited) {
        Stop-Process -Id $workerProcess.Id -Force
    }
    if ($mysqlProcess -and -not $mysqlProcess.HasExited) {
        & $mysqlAdmin -u root shutdown
    }
}
```

## 4. 分步 curl 示例

### 4.1 健康检查

```powershell
curl.exe http://127.0.0.1:8080/api/health
```

预期：

```json
{"status":"ok"}
```

### 4.2 注册

```powershell
curl.exe -X POST `
  -H "Content-Type: application/json" `
  -d "{\"username\":\"alice\",\"password\":\"secret123\"}" `
  http://127.0.0.1:8080/api/auth/register
```

### 4.3 登录并保存 Cookie

```powershell
curl.exe -c cookies.txt -X POST `
  -H "Content-Type: application/json" `
  -d "{\"username\":\"alice\",\"password\":\"secret123\"}" `
  http://127.0.0.1:8080/api/auth/login
```

### 4.4 当前用户

```powershell
curl.exe -b cookies.txt http://127.0.0.1:8080/api/auth/me
```

### 4.5 提交判题

```powershell
curl.exe -b cookies.txt -X POST `
  -H "Content-Type: application/json" `
  -d "{\"code\":\"#include <iostream>\\nint main(){return 0;}\"}" `
  http://127.0.0.1:8080/api/problems/1/submit
```

注意：提交判题必须同时运行 `web_server` 和 `judge_worker`。

## 5. 预期结果

| 测试项 | 预期状态码 |
| --- | --- |
| 健康检查 | `200` |
| 题目列表公开访问 | `200` |
| 未登录访问题目详情 | `401` |
| 注册用户 | `201` |
| 普通用户登录 | `200` |
| 当前用户 | `200` |
| 登录后查看题目详情 | `200` |
| 运行样例 | `200` |
| 第一次提交 | `200` |
| 短时间内第二次提交 | `429` |
| 普通用户访问管理端 | `403` |
| 退出登录 | `200` |
| 退出后访问当前用户 | `401` |
| 管理员登录 | `200` |
| 管理员题目列表 | `200` |
| 新增题目 | `201` |
| 修改题目 | `200` |
| 导入测试用例 | `200` |
| 删除题目 | `200` |
| 查询已删除题目 | `404` |

## 6. 清理说明

脚本执行完毕后会自动：

- 删除临时测试用户。
- 删除临时测试题目。
- 关闭 `web_server`。
- 关闭 `judge_worker`。
- 关闭 MySQL。

如果脚本中途失败，仍需手动检查并关闭进程：

```powershell
Get-Process web_server,judge_worker,mysqld -ErrorAction SilentlyContinue
```

必要时关闭：

```powershell
Stop-Process -Name web_server,judge_worker -Force
D:\codexworks\cppojvibecoding\.tools\mysql\bin\mysqladmin.exe -u root shutdown
```

## 7. 常见问题

### 7.1 提交接口返回 503

说明 `judge_worker` 没有运行，或判题等待超过 10 秒。检查：

```powershell
Get-Content build\judge_worker.log
Get-Content build\judge_worker_error.log
```

### 7.2 注册或登录失败

检查 MySQL 是否启动：

```powershell
Get-Process mysqld
```

检查 Web 服务日志：

```powershell
Get-Content build\web_server_error.log
```

### 7.3 连续提交返回 429

这是正常的限流行为。默认每个用户每 3 秒只能提交一次，等待 `retry_after_ms` 后重试。

### 7.4 控制台中文显示乱码

接口实际返回 UTF-8 JSON，如果 PowerShell 控制台显示乱码，可以读取原始响应文件并用 UTF-8 显示：

```powershell
Get-Content -Raw -Encoding UTF8 build\api_response.json
```
