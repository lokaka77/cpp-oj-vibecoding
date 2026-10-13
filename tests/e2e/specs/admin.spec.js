"use strict";

const { test, expect } = require("@playwright/test");
const { login } = require("../helpers");

test.describe("管理员验收", () => {
    test("管理员登录后可见管理入口与用户目录", async ({ page }) => {
        await login(page, "admin", "admin123");

        await expect(page.locator("#user-role")).toHaveText("管理员");
        await expect(page.locator("#admin-nav-link")).toBeVisible();
        await expect(page.locator("#admin-users-nav-link")).toBeVisible();

        await page.locator("#admin-users-nav-link").click();
        await expect(page).toHaveURL(/admin-users\.html$/);
        await expect(page.locator("#user-count")).toHaveText(/共 \d+ 个账号/);

        const rows = page.locator("#user-items .user-row");
        expect(await rows.count()).toBeGreaterThanOrEqual(20);
        await expect(page.locator("#user-items .protected-label"))
            .toHaveText("受保护");
    });

    test("管理员可新建并删除题目", async ({ page }) => {
        await login(page, "admin", "admin123");
        await page.goto("/admin.html");
        await expect(page.locator("#admin-problem-list .admin-problem-item"))
            .toHaveCount(10);

        const title = "E2E 临时题目 " + Date.now();

        await page.locator("#new-problem-button").click();
        await page.locator("#admin-title").fill(title);
        await page.locator("#admin-difficulty").selectOption("medium");
        await page.locator("#admin-description")
            .fill("由端到端测试创建，随后立即删除。");
        await page.locator("#save-problem-button").click();

        await expect(page.locator("#admin-status"))
            .toHaveText("题目创建成功");
        await expect(page.locator("#admin-problem-list .admin-problem-item"))
            .toHaveCount(11);
        await expect(page.locator("#admin-problem-list"))
            .toContainText(title);

        await page.locator("#delete-problem-button").click();
        await expect(page.locator("#confirm-dialog")).toBeVisible();
        await page.locator("#confirm-delete-button").click();

        await expect(page.locator("#admin-problem-list .admin-problem-item"))
            .toHaveCount(10);
        await expect(page.locator("#admin-problem-list"))
            .not.toContainText(title);
    });
});
