# 导入 10 道 C++ 练习题（含难度与测试用例）。
# 用法：powershell -ExecutionPolicy Bypass -File scripts\seed_problems.ps1
# 说明：以题目标题为准，已存在则更新描述与难度并替换测试用例，不存在则新建。

$ErrorActionPreference = 'Stop'

$root = Split-Path $PSScriptRoot -Parent
$build = Join-Path $root 'build'
$base = 'http://127.0.0.1:8080'

$loginFile = Join-Path $build 'seed_admin.json'
$cookieFile = Join-Path $build 'seed_admin_cookie.txt'
$payloadFile = Join-Path $build 'seed_payload.json'
$caseFile = Join-Path $build 'seed_cases.json'
$responseFile = Join-Path $build 'seed_response.json'

function Write-JsonFile {
    param([string]$Path, [object]$Value)
    [System.IO.File]::WriteAllText(
        $Path, ($Value | ConvertTo-Json -Depth 10 -Compress))
}

function Read-JsonFile {
    param([string]$Path)
    return (Get-Content $Path -Raw -Encoding UTF8 | ConvertFrom-Json)
}

$problems = @(
    @{
        title = 'A+B Problem'
        difficulty = 'easy'
        description = @'
给定两个整数 a 和 b，计算它们的和。

输入格式
一行包含两个整数 a 和 b，用空格分隔（-1000000000 ≤ a, b ≤ 1000000000）。

输出格式
输出一个整数，表示 a + b 的值。
'@
        test_cases = @(
            @{ input = "1 2`n"; expected_output = "3`n"; is_sample = $true },
            @{ input = "100 -50`n"; expected_output = "50`n"; is_sample = $true },
            @{ input = "0 0`n"; expected_output = "0`n"; is_sample = $false },
            @{ input = "-1000000000 1000000000`n"; expected_output = "0`n"; is_sample = $false },
            @{ input = "999999999 1000000000`n"; expected_output = "1999999999`n"; is_sample = $false }
        )
    },
    @{
        title = '两数最大值'
        difficulty = 'easy'
        description = @'
读入两个整数，输出其中较大的一个。

输入格式
一行包含两个整数 a 和 b，用空格分隔（-1000000000 ≤ a, b ≤ 1000000000）。

输出格式
输出较大的那个整数；两个数相等时输出该数。
'@
        test_cases = @(
            @{ input = "3 5`n"; expected_output = "5`n"; is_sample = $true },
            @{ input = "-7 -2`n"; expected_output = "-2`n"; is_sample = $true },
            @{ input = "7 7`n"; expected_output = "7`n"; is_sample = $false },
            @{ input = "0 -1`n"; expected_output = "0`n"; is_sample = $false },
            @{ input = "-1000000000 1000000000`n"; expected_output = "1000000000`n"; is_sample = $false }
        )
    },
    @{
        title = '阶乘'
        difficulty = 'easy'
        description = @'
给定正整数 n，计算 n 的阶乘 n! = 1 × 2 × … × n。

输入格式
一行包含一个整数 n（1 ≤ n ≤ 12）。

输出格式
输出 n! 的值。
'@
        test_cases = @(
            @{ input = "5`n"; expected_output = "120`n"; is_sample = $true },
            @{ input = "1`n"; expected_output = "1`n"; is_sample = $true },
            @{ input = "7`n"; expected_output = "5040`n"; is_sample = $false },
            @{ input = "10`n"; expected_output = "3628800`n"; is_sample = $false },
            @{ input = "12`n"; expected_output = "479001600`n"; is_sample = $false }
        )
    },
    @{
        title = '回文串判断'
        difficulty = 'easy'
        description = @'
给定一个只包含小写字母的字符串，判断它是否为回文串（正着读和倒着读相同）。

输入格式
一行包含一个字符串 s（1 ≤ |s| ≤ 100）。

输出格式
如果 s 是回文串输出 YES，否则输出 NO。
'@
        test_cases = @(
            @{ input = "level`n"; expected_output = "YES`n"; is_sample = $true },
            @{ input = "hello`n"; expected_output = "NO`n"; is_sample = $true },
            @{ input = "a`n"; expected_output = "YES`n"; is_sample = $false },
            @{ input = "abba`n"; expected_output = "YES`n"; is_sample = $false },
            @{ input = "abca`n"; expected_output = "NO`n"; is_sample = $false }
        )
    },
    @{
        title = '排序输出'
        difficulty = 'medium'
        description = @'
给定 n 个整数，把它们按从小到大的顺序输出。

输入格式
第一行包含一个整数 n（1 ≤ n ≤ 1000）。
第二行包含 n 个整数，用空格分隔（-100000 ≤ ai ≤ 100000）。

输出格式
一行输出排序后的 n 个整数，用单个空格分隔。
'@
        test_cases = @(
            @{ input = "5`n3 1 4 1 5`n"; expected_output = "1 1 3 4 5`n"; is_sample = $true },
            @{ input = "1`n42`n"; expected_output = "42`n"; is_sample = $true },
            @{ input = "3`n-1 0 -1`n"; expected_output = "-1 -1 0`n"; is_sample = $false },
            @{ input = "4`n2 2 2 2`n"; expected_output = "2 2 2 2`n"; is_sample = $false },
            @{ input = "6`n100000 99999 50000 1 0 -100000`n"; expected_output = "-100000 0 1 50000 99999 100000`n"; is_sample = $false }
        )
    },
    @{
        title = '最大公约数'
        difficulty = 'medium'
        description = @'
给定两个正整数 a 和 b，求它们的最大公约数。

输入格式
一行包含两个正整数 a 和 b，用空格分隔（1 ≤ a, b ≤ 1000000000）。

输出格式
输出 a 和 b 的最大公约数。
'@
        test_cases = @(
            @{ input = "12 18`n"; expected_output = "6`n"; is_sample = $true },
            @{ input = "100 75`n"; expected_output = "25`n"; is_sample = $true },
            @{ input = "7 13`n"; expected_output = "1`n"; is_sample = $false },
            @{ input = "270 192`n"; expected_output = "6`n"; is_sample = $false },
            @{ input = "1000000000 999999999`n"; expected_output = "1`n"; is_sample = $false }
        )
    },
    @{
        title = '括号匹配'
        difficulty = 'medium'
        description = @'
给定一个只由小括号、中括号和大括号组成的字符串，判断其中的括号是否正确匹配（左右类型对应且嵌套顺序正确）。

输入格式
一行包含一个字符串 s（1 ≤ |s| ≤ 1000），只包含字符 ()[]{}。

输出格式
如果括号完全匹配输出 YES，否则输出 NO。
'@
        test_cases = @(
            @{ input = "()[]{}`n"; expected_output = "YES`n"; is_sample = $true },
            @{ input = "([)]`n"; expected_output = "NO`n"; is_sample = $true },
            @{ input = "{[]}`n"; expected_output = "YES`n"; is_sample = $false },
            @{ input = "(]`n"; expected_output = "NO`n"; is_sample = $false },
            @{ input = "((()))`n"; expected_output = "YES`n"; is_sample = $false }
        )
    },
    @{
        title = '快速幂取模'
        difficulty = 'hard'
        description = @'
给定整数 a、b 和正整数 m，计算 a 的 b 次方对 m 取模的结果。

输入格式
一行包含三个整数 a、b、m，用空格分隔（0 ≤ a, b ≤ 1000000000，1 ≤ m ≤ 1000000000）。

输出格式
输出 a^b mod m 的值。

说明
b 可能很大，请使用快速幂算法，并使用 64 位整数保存中间结果。
'@
        test_cases = @(
            @{ input = "2 10 1000`n"; expected_output = "24`n"; is_sample = $true },
            @{ input = "3 13 7`n"; expected_output = "3`n"; is_sample = $true },
            @{ input = "0 5 7`n"; expected_output = "0`n"; is_sample = $false },
            @{ input = "1 1000000000 997`n"; expected_output = "1`n"; is_sample = $false },
            @{ input = "987654321 12345 1000000007`n"; expected_output = "678872759`n"; is_sample = $false }
        )
    },
    @{
        title = '最长递增子序列'
        difficulty = 'hard'
        description = @'
给定一个长度为 n 的整数序列，求它的最长严格递增子序列的长度。

输入格式
第一行包含一个整数 n（1 ≤ n ≤ 1000）。
第二行包含 n 个整数，用空格分隔（-100000 ≤ ai ≤ 100000）。

输出格式
输出最长严格递增子序列的长度。

说明
子序列不要求连续，只需保持原有先后顺序，且元素严格递增。
'@
        test_cases = @(
            @{ input = "8`n10 9 2 5 3 7 101 18`n"; expected_output = "4`n"; is_sample = $true },
            @{ input = "5`n1 2 3 4 5`n"; expected_output = "5`n"; is_sample = $true },
            @{ input = "1`n5`n"; expected_output = "1`n"; is_sample = $false },
            @{ input = "5`n5 4 3 2 1`n"; expected_output = "1`n"; is_sample = $false },
            @{ input = "6`n2 2 2 2 2 2`n"; expected_output = "1`n"; is_sample = $false }
        )
    },
    @{
        title = '01 背包'
        difficulty = 'hard'
        description = @'
有 n 件物品和一个容量为 W 的背包，第 i 件物品的重量为 wi、价值为 vi。每件物品最多选一次，求背包能装下的最大总价值。

输入格式
第一行包含两个整数 n 和 W（1 ≤ n ≤ 100，1 ≤ W ≤ 1000）。
接下来 n 行，每行包含两个整数 wi 和 vi（1 ≤ wi ≤ 1000，1 ≤ vi ≤ 1000）。

输出格式
输出不超过背包容量的前提下能获得的最大总价值。
'@
        test_cases = @(
            @{ input = "4 5`n1 2`n2 4`n3 4`n4 5`n"; expected_output = "8`n"; is_sample = $true },
            @{ input = "1 10`n5 100`n"; expected_output = "100`n"; is_sample = $true },
            @{ input = "2 3`n4 100`n5 100`n"; expected_output = "0`n"; is_sample = $false },
            @{ input = "3 6`n2 3`n2 4`n3 5`n"; expected_output = "9`n"; is_sample = $false },
            @{ input = "2 5`n3 30`n3 40`n"; expected_output = "40`n"; is_sample = $false }
        )
    }
)

Write-JsonFile $loginFile @{ username = 'admin'; password = 'admin123' }
$status = & curl.exe -s -o $responseFile -w '%{http_code}' `
    -c $cookieFile `
    -X POST -H 'Content-Type: application/json' `
    --data-binary "@$loginFile" `
    "$base/api/auth/login"
if ($status -ne '200') {
    throw "管理员登录失败（HTTP $status），请先执行 scripts\start_local.ps1"
}

$null = & curl.exe -s -o $responseFile "$base/api/problems"
$existingByTitle = @{}
foreach ($item in (Read-JsonFile $responseFile).problems) {
    $existingByTitle[$item.title] = $item.id
}

foreach ($problem in $problems) {
    Write-JsonFile $payloadFile @{
        title = $problem.title
        description = $problem.description
        difficulty = $problem.difficulty
    }

    if ($existingByTitle.ContainsKey($problem.title)) {
        $problemId = $existingByTitle[$problem.title]
        $status = & curl.exe -s -o $responseFile -w '%{http_code}' `
            -b $cookieFile `
            -X PUT -H 'Content-Type: application/json' `
            --data-binary "@$payloadFile" `
            "$base/api/admin/problems/$problemId"
        if ($status -ne '200') {
            throw "更新题目失败：$($problem.title)（HTTP $status）"
        }
        $action = '更新'
    } else {
        $status = & curl.exe -s -o $responseFile -w '%{http_code}' `
            -b $cookieFile `
            -X POST -H 'Content-Type: application/json' `
            --data-binary "@$payloadFile" `
            "$base/api/admin/problems"
        if ($status -ne '201') {
            throw "创建题目失败：$($problem.title)（HTTP $status）"
        }
        $problemId = (Read-JsonFile $responseFile).id
        $action = '新建'
    }

    Write-JsonFile $caseFile @{ test_cases = $problem.test_cases }
    $status = & curl.exe -s -o $responseFile -w '%{http_code}' `
        -b $cookieFile `
        -X POST -H 'Content-Type: application/json' `
        --data-binary "@$caseFile" `
        "$base/api/admin/problems/$problemId/testcases"
    if ($status -ne '200') {
        throw "导入测试用例失败：$($problem.title)（HTTP $status）"
    }

    Write-Host ("{0}  #{1}  [{2}]  {3}  用例 {4} 组" -f `
        $action, $problemId, $problem.difficulty, $problem.title, `
        $problem.test_cases.Count)
}

$null = & curl.exe -s -o $responseFile "$base/api/problems"
$total = (Read-JsonFile $responseFile).problems.Count
Write-Host ''
Write-Host "题库导入完成，当前题目总数：$total"
