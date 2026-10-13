// @ts-check
const { defineConfig } = require("@playwright/test");

module.exports = defineConfig({
    testDir: "./specs",
    timeout: 60 * 1000,
    expect: {
        timeout: 10 * 1000
    },
    fullyParallel: false,
    workers: 1,
    retries: 0,
    reporter: [
        ["list"],
        ["html", { open: "never", outputFolder: "report" }]
    ],
    use: {
        baseURL: "http://127.0.0.1:8080",
        channel: "msedge",
        headless: true,
        viewport: { width: 1440, height: 900 },
        actionTimeout: 15 * 1000,
        navigationTimeout: 20 * 1000,
        trace: "retain-on-failure",
        screenshot: "only-on-failure"
    }
});
