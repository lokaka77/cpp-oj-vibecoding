"use strict";

const { test, expect } = require("@playwright/test");
const { login } = require("../helpers");

// 五种判题结论各用一个独立账号提交，避开单用户 3 秒提交频率限制。
const JUDGE_CASES = [
    {
        verdict: "AC",
        account: "student05",
        message: "全部测试点通过",
        code: [
            "#include <iostream>",
            "int main() {",
            "    long long a = 0;",
            "    long long b = 0;",
            "    std::cin >> a >> b;",
            "    std::cout << a + b << std::endl;",
            "    return 0;",
            "}"
        ]
    },
    {
        verdict: "WA",
        account: "student06",
        message: "输出与期望不一致",
        code: [
            "#include <iostream>",
            "int main() {",
            "    std::cout << 1 << std::endl;",
            "    return 0;",
            "}"
        ]
    },
    {
        verdict: "TLE",
        account: "student07",
        message: "运行超时",
        code: [
            "#include <iostream>",
            "int main() {",
            "    while (true) {",
            "    }",
            "    return 0;",
            "}"
        ]
    },
    {
        verdict: "RE",
        account: "student08",
        message: "运行时错误或内存超限",
        code: [
            "#include <iostream>",
            "int main() {",
            "    int* pointer = nullptr;",
            "    *pointer = 42;",
            "    return 0;",
            "}"
        ]
    },
    {
        verdict: "CE",
        account: "student09",
        message: "编译错误",
        code: [
            "#include <iostream>",
            "int main() {",
            "    this is not valid c++",
            "}"
        ]
    }
];

for (const judgeCase of JUDGE_CASES) {
    test(`提交代码返回 ${judgeCase.verdict}`, async ({ page }) => {
        await login(page, judgeCase.account, "student123");
        await page.goto("/problem.html?id=1");
        await expect(page.locator("#workspace-editor")).toBeVisible();

        await page.locator("#code-editor").fill(judgeCase.code.join("\n"));
        await page.locator("#submit-button").click();

        await expect(page.locator("#result-verdict"))
            .toHaveText(judgeCase.verdict, { timeout: 30 * 1000 });
        await expect(page.locator("#result-message"))
            .toHaveText(judgeCase.message);
    });
}
