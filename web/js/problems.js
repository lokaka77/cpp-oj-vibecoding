(function () {
    "use strict";

    function mapError(error) {
        switch (error.code) {
        case "NETWORK_ERROR":
            return "无法连接服务器，请确认后端已启动";
        case "INTERNAL_ERROR":
            return "题目加载失败，请稍后重试";
        default:
            return error.message || "题目加载失败";
        }
    }

    function initProblemsPage() {
        if (!window.OJApi) {
            return;
        }

        const elements = {
            userName: document.getElementById("user-name"),
            userRole: document.getElementById("user-role"),
            adminNav: document.getElementById("admin-nav-link"),
            logoutButton: document.getElementById("logout-button"),
            search: document.getElementById("problem-search"),
            count: document.getElementById("problem-count"),
            loading: document.getElementById("loading-state"),
            list: document.getElementById("problem-list"),
            items: document.getElementById("problem-items"),
            empty: document.getElementById("empty-state"),
            error: document.getElementById("error-state"),
            errorMessage: document.getElementById("error-message"),
            retry: document.getElementById("retry-button")
        };

        const state = {
            problems: [],
            filtered: [],
            query: "",
            loading: false
        };

        if (window.CodeHunterBackground) {
            window.CodeHunterBackground.init("tech-canvas");
        }

        function showOnly(element) {
            [elements.loading, elements.list, elements.empty, elements.error]
                .forEach(function (candidate) {
                    candidate.hidden = candidate !== element;
                });
        }

        function updateCount() {
            const total = state.problems.length;
            const visible = state.filtered.length;
            elements.count.textContent = state.query
                ? "显示 " + visible + " / " + total + " 道题"
                : "共 " + total + " 道题";
        }

        function createProblemItem(problem) {
            const item = document.createElement("article");
            item.className = "problem-item";
            item.setAttribute("role", "listitem");

            const id = document.createElement("span");
            id.className = "problem-id";
            id.textContent = "#" + String(problem.id).padStart(3, "0");

            const title = document.createElement("div");
            title.className = "problem-title";
            const heading = document.createElement("h2");
            heading.textContent = problem.title;
            title.appendChild(heading);

            const status = document.createElement("span");
            status.className = "problem-status";
            status.textContent = "可作答";

            const action = document.createElement("a");
            action.className = "problem-action";
            action.href = "./problem.html?id=" + encodeURIComponent(problem.id);
            action.textContent = "开始解题";
            action.setAttribute("aria-label", "开始解题：" + problem.title);

            item.appendChild(id);
            item.appendChild(title);
            item.appendChild(status);
            item.appendChild(action);
            return item;
        }

        function renderProblems() {
            elements.items.replaceChildren();

            if (state.filtered.length === 0) {
                elements.empty.hidden = false;
                elements.list.hidden = true;
                updateCount();
                showOnly(elements.empty);
                return;
            }

            const fragment = document.createDocumentFragment();
            state.filtered.forEach(function (problem) {
                fragment.appendChild(createProblemItem(problem));
            });
            elements.items.appendChild(fragment);
            updateCount();
            showOnly(elements.list);
        }

        function applyFilter() {
            const query = state.query.trim().toLowerCase();

            if (!query) {
                state.filtered = state.problems.slice();
            } else {
                state.filtered = state.problems.filter(function (problem) {
                    return problem.title.toLowerCase().includes(query) ||
                           String(problem.id).includes(query);
                });
            }

            renderProblems();
        }

        async function loadProblems() {
            if (state.loading) {
                return;
            }

            state.loading = true;
            showOnly(elements.loading);

            try {
                const data = await window.OJApi.listProblems();
                state.problems = data && data.problems ? data.problems : [];
                applyFilter();
            } catch (error) {
                elements.errorMessage.textContent = mapError(error);
                showOnly(elements.error);
            } finally {
                state.loading = false;
            }
        }

        async function checkAuthentication() {
            try {
                const user = await window.OJApi.me();
                elements.userName.textContent = user.username;

                if (user.role === "admin") {
                    elements.userRole.textContent = "管理员";
                    elements.userRole.hidden = false;
                    elements.adminNav.hidden = false;
                }

                return true;
            } catch (error) {
                window.location.replace("./login.html");
                return false;
            }
        }

        elements.search.addEventListener("input", function (event) {
            state.query = event.target.value;
            applyFilter();
        });

        elements.retry.addEventListener("click", loadProblems);

        elements.logoutButton.addEventListener("click", async function () {
            elements.logoutButton.disabled = true;
            try {
                await window.OJApi.logout();
            } finally {
                window.location.replace("./login.html");
            }
        });

        document.addEventListener("keydown", function (event) {
            if (event.key === "/" &&
                document.activeElement !== elements.search &&
                document.activeElement.tagName !== "INPUT") {
                event.preventDefault();
                elements.search.focus();
            }
        });

        checkAuthentication().then(function (authenticated) {
            if (authenticated) {
                loadProblems();
            }
        });
    }

    document.addEventListener("DOMContentLoaded", initProblemsPage);
})();
