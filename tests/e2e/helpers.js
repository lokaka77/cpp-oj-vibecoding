"use strict";

async function login(page, username, password) {
    await page.goto("/login.html");
    await page.locator("#username").fill(username);
    await page.locator("#password").fill(password);
    await page.locator("#submit-button").click();
    await page.waitForURL("**/problems.html");
}

module.exports = { login };
