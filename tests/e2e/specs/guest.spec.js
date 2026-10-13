"use strict";

const { test, expect } = require("@playwright/test");

test.describe("游客验收", () => {
    test("落地页展示品牌与开始刷题入口", async ({ page }) => {
        await page.goto("/index.html");

        await expect(page.locator(".landing-hero h1")).toHaveText("CodeHunter");
        await expect(page.locator("#primary-cta")).toBeVisible();
        await expect(page.locator("#primary-cta"))
            .toHaveAttribute("href", "./problems.html");
        await expect(page.locator("#login-link")).toBeVisible();
    });

    test("游客可直接浏览题库，不被强制跳转登录页", async ({ page }) => {
        await page.goto("/problems.html");

        await expect(page).toHaveURL(/problems\.html$/);
        await expect(page.locator("#problem-items .problem-item"))
            .toHaveCount(10);
        await expect(page.locator("#login-link")).toBeVisible();
        await expect(page.locator("#logout-button")).toBeHidden();
        await expect(page.locator("#problem-count")).toHaveText("共 10 道题");
    });

    test("按难度筛选只显示对应题目", async ({ page }) => {
        await page.goto("/problems.html");
        await expect(page.locator("#problem-items .problem-item"))
            .toHaveCount(10);

        await page.locator('.filter-tab[data-difficulty="hard"]').click();
        await expect(page.locator("#problem-count"))
            .toHaveText("显示 3 / 10 道题");
        await expect(page.locator("#problem-items .problem-item"))
            .toHaveCount(3);
        await expect(page.locator("#problem-items .difficulty-badge"))
            .toHaveText(["困难", "困难", "困难"]);

        await page.locator('.filter-tab[data-difficulty="easy"]').click();
        await expect(page.locator("#problem-count"))
            .toHaveText("显示 4 / 10 道题");

        await page.locator('.filter-tab[data-difficulty="all"]').click();
        await expect(page.locator("#problem-items .problem-item"))
            .toHaveCount(10);
    });

    test("游客可从题库进入题目详情预览样例", async ({ page }) => {
        await page.goto("/problems.html");
        await page.locator("#problem-items .problem-action").first().click();

        await expect(page).toHaveURL(/problem\.html\?id=1$/);
        await expect(page.locator("#problem-title")).toHaveText("A+B Problem");
        await expect(page.locator("#problem-difficulty")).toHaveText("简单");
        await expect(page.locator("#problem-description")).not.toBeEmpty();
        await expect(page.locator("#sample-list .sample-item").first())
            .toBeVisible();

        await expect(page.locator("#workspace-editor")).toBeHidden();
        await expect(page.locator("#workspace-guest")).toBeVisible();
    });

    test("左上角品牌可返回落地页", async ({ page }) => {
        await page.goto("/problems.html");
        await page.locator(".app-brand").click();

        await expect(page).toHaveURL(/index\.html$/);
        await expect(page.locator(".landing-hero h1")).toHaveText("CodeHunter");
    });
});
