(function () {
    "use strict";

    function initLandingPage() {
        if (window.CodeHunterBackground) {
            window.CodeHunterBackground.init("tech-canvas");
        }

        if (!window.OJApi) {
            return;
        }

        const elements = {
            userName: document.getElementById("user-name"),
            userRole: document.getElementById("user-role"),
            adminNav: document.getElementById("admin-nav-link"),
            adminUsersNav: document.getElementById("admin-users-nav-link"),
            loginLink: document.getElementById("login-link"),
            logoutButton: document.getElementById("logout-button"),
            primaryCta: document.getElementById("primary-cta"),
            secondaryCta: document.getElementById("secondary-cta"),
            problemStat: document.getElementById("stat-problems")
        };

        function showAuthenticatedUser(user) {
            elements.userName.textContent = user.username;
            elements.userName.hidden = false;
            elements.loginLink.hidden = true;
            elements.logoutButton.hidden = false;
            elements.primaryCta.textContent = "继续刷题";
            elements.secondaryCta.hidden = true;

            if (user.role === "admin") {
                elements.userRole.textContent = "管理员";
                elements.userRole.hidden = false;
                elements.adminNav.hidden = false;
                elements.adminUsersNav.hidden = false;
            }
        }

        async function loadProblemCount() {
            try {
                const data = await window.OJApi.listProblems();
                const problems = data && data.problems ? data.problems : [];
                elements.problemStat.textContent = String(problems.length);
            } catch (error) {
                elements.problemStat.textContent = "—";
            }
        }

        window.OJApi.me().then(showAuthenticatedUser).catch(function () {
            // 未登录时保持游客状态
        });

        elements.logoutButton.addEventListener("click", async function () {
            elements.logoutButton.disabled = true;
            try {
                await window.OJApi.logout();
            } finally {
                window.location.reload();
            }
        });

        loadProblemCount();
    }

    document.addEventListener("DOMContentLoaded", initLandingPage);
})();
