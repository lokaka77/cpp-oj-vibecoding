"use strict";

const { test, expect } = require("@playwright/test");
const { login } = require("../helpers");

test.describe("学生验收", () => {
    test("登录后进入题库并显示账号信息", async ({ page }) => {
        await login(page, "student02", "student123");

        await expect(page.locator("#user-name")).toHaveText("student02");
        await expect(page.locator("#logout-button")).toBeVisible();
        await expect(page.locator("#login-link")).toBeHidden();
        await expect(page.locator("#admin-nav-link")).toBeHidden();
    });

    test("运行样例并提交判题返回 AC", async ({ page }) => {
        await login(page, "student03", "student123");
        await page.goto("/problem.html?id=1");

        await expect(page.locator("#workspace-editor")).toBeVisible();
        await expect(page.locator("#workspace-guest")).toBeHidden();
        await page.locator("#code-editor").fill([
            "#include <iostream>",
            "int main() {",
            "    long long a = 0;",
            "    long long b = 0;",
            "    std::cin >> a >> b;",
            "    std::cout << a + b << std::endl;",
            "    return 0;",
            "}"
        ].join("\n"));

        await page.locator("#run-button").click();
        await expect(page.locator("#result-verdict"))
            .toHaveText("PASS", { timeout: 30 * 1000 });

        await page.locator("#submit-button").click();
        await expect(page.locator("#result-verdict"))
            .toHaveText("AC", { timeout: 30 * 1000 });
        await expect(page.locator("#result-message"))
            .toHaveText("全部测试点通过");
    });

    test("退出登录后回到登录页", async ({ page }) => {
        await login(page, "student04", "student123");
        await page.locator("#logout-button").click();

        await expect(page).toHaveURL(/login\.html$/);
    });
});
