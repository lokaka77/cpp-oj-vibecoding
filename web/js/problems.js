(function () {
    "use strict";

    const DIFFICULTY_LABELS = {
        easy: "简单",
        medium: "中等",
        hard: "困难"
    };

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
            adminUsersNav: document.getElementById("admin-users-nav-link"),
            loginLink: document.getElementById("login-link"),
            logoutButton: document.getElementById("logout-button"),
            search: document.getElementById("problem-search"),
            filterTabs: document.querySelectorAll(".filter-tab"),
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
            difficulty: "all",
            loading: false,
            solvedIds: new Set()
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
            const filtering = Boolean(state.query) ||
                state.difficulty !== "all";
            elements.count.textContent = filtering
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

            const solved = state.solvedIds.has(problem.id);
            if (solved) {
                const egg = document.createElement("span");
                egg.className = "solved-egg";
                egg.setAttribute("role", "img");
                egg.setAttribute("aria-label", "已通过");
                egg.innerHTML =
                    '<svg viewBox="0 0 24 24" aria-hidden="true">' +
                    '<path d="M9 4.6C9.8 3.5 10.8 3 12 3s2.2.5 3 1.6' +
                    'c1.6 2.2 2.5 5.3 2.5 8.4 0 5-2.4 8.5-5.5 8.5' +
                    'S6.5 18 6.5 13c0-3.1.9-6.2 2.5-8.4Z"></path>' +
                    '<path class="egg-shine" d="M9.6 8.4' +
                    'c-.7 1.4-1.1 3-1.1 4.7"></path>' +
                    '</svg>';
                title.appendChild(egg);
            }

            const difficulty = document.createElement("span");
            difficulty.className = "difficulty-badge";
            difficulty.dataset.level = problem.difficulty || "easy";
            difficulty.textContent =
                DIFFICULTY_LABELS[problem.difficulty] || "简单";

            const status = document.createElement("span");
            status.className = "problem-status";
            status.textContent = solved ? "已通过" : "可作答";
            status.classList.toggle("is-solved", solved);

            item.classList.toggle("is-solved", solved);

            const action = document.createElement("a");
            action.className = "problem-action";
            action.href = "./problem.html?id=" + encodeURIComponent(problem.id);
            action.textContent = "开始解题";
            action.setAttribute("aria-label", "开始解题：" + problem.title);

            item.appendChild(id);
            item.appendChild(title);
            item.appendChild(difficulty);
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

            state.filtered = state.problems.filter(function (problem) {
                if (state.difficulty !== "all" &&
                    problem.difficulty !== state.difficulty) {
                    return false;
                }

                if (!query) {
                    return true;
                }

                return problem.title.toLowerCase().includes(query) ||
                       String(problem.id).includes(query);
            });

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

        async function loadSolvedProblems() {
            try {
                const data = await window.OJApi.listSolvedProblems();
                state.solvedIds = new Set(
                    data && data.problem_ids ? data.problem_ids : []
                );
            } catch (error) {
                state.solvedIds = new Set();
            }
        }

        async function resolveUser() {
            try {
                const user = await window.OJApi.me();
                elements.userName.textContent = user.username;
                elements.userName.hidden = false;
                elements.logoutButton.hidden = false;

                if (user.role === "admin") {
                    elements.userRole.textContent = "管理员";
                    elements.userRole.hidden = false;
                    elements.adminNav.hidden = false;
                    elements.adminUsersNav.hidden = false;
                }

                return user;
            } catch (error) {
                if (elements.loginLink) {
                    elements.loginLink.hidden = false;
                }
                return null;
            }
        }

        elements.search.addEventListener("input", function (event) {
            state.query = event.target.value;
            applyFilter();
        });

        function setDifficulty(difficulty) {
            state.difficulty = difficulty;
            elements.filterTabs.forEach(function (tab) {
                const active = tab.dataset.difficulty === difficulty;
                tab.classList.toggle("is-active", active);
                tab.setAttribute("aria-selected", String(active));
            });
            applyFilter();
        }

        elements.filterTabs.forEach(function (tab) {
            tab.addEventListener("click", function () {
                setDifficulty(tab.dataset.difficulty);
            });

            tab.addEventListener("keydown", function (event) {
                if (event.key !== "ArrowLeft" &&
                    event.key !== "ArrowRight") {
                    return;
                }

                event.preventDefault();
                const tabs = Array.prototype.slice.call(elements.filterTabs);
                const offset = event.key === "ArrowRight" ? 1 : -1;
                const nextTab =
                    tabs[(tabs.indexOf(tab) + offset + tabs.length) %
                         tabs.length];
                nextTab.focus();
                setDifficulty(nextTab.dataset.difficulty);
            });
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

        resolveUser().then(async function (user) {
            if (user) {
                await loadSolvedProblems();
            }
            loadProblems();
        });
    }

    document.addEventListener("DOMContentLoaded", initProblemsPage);
})();
