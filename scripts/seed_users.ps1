# 批量创建 20 个测试用户（student01 ~ student20）。
# 用法：powershell -ExecutionPolicy Bypass -File scripts\seed_users.ps1
# 说明：通过注册接口创建，密码按真实流程做 Argon2id 哈希；已存在的用户自动跳过。

$ErrorActionPreference = 'Stop'

$root = Split-Path $PSScriptRoot -Parent
$build = Join-Path $root 'build'
$base = 'http://127.0.0.1:8080'
$password = 'student123'
$count = 20

$payloadFile = Join-Path $build 'seed_user.json'
$responseFile = Join-Path $build 'seed_user_response.json'

$created = 0
$existing = 0

for ($index = 1; $index -le $count; $index++) {
    $username = 'student{0:D2}' -f $index
    [System.IO.File]::WriteAllText(
        $payloadFile,
        (@{ username = $username; password = $password } |
            ConvertTo-Json -Compress))

    $status = & curl.exe -s -o $responseFile -w '%{http_code}' `
        -X POST -H 'Content-Type: application/json' `
        --data-binary "@$payloadFile" `
        "$base/api/auth/register"

    switch ($status) {
        '201' {
            $created++
            Write-Host "新建   $username"
        }
        '409' {
            $existing++
            Write-Host "已存在 $username"
        }
        default {
            throw "创建用户失败：$username（HTTP $status）"
        }
    }
}

Write-Host ''
Write-Host "测试用户准备完成：新建 $created 个，已存在 $existing 个，共 $count 个"
Write-Host "用户名：student01 ~ student$('{0:D2}' -f $count)    密码：$password"
