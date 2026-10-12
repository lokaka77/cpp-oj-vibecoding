(function () {
    "use strict";

    function mapError(error) {
        switch (error.code) {
        case "NOT_AUTHENTICATED":
            return "登录状态已失效，请重新登录";
        case "ADMIN_REQUIRED":
            return "当前账号没有管理员权限";
        case "USER_NOT_FOUND":
            return "用户不存在或已被删除";
        case "ADMIN_ACCOUNT_PROTECTED":
            return "管理员账号不允许删除";
        case "NETWORK_ERROR":
            return "无法连接服务器，请确认后端已启动";
        case "INTERNAL_ERROR":
            return "服务暂时不可用，请稍后重试";
        default:
            return error.message || "请求失败";
        }
    }

    function initAdminUsersPage() {
        if (!window.OJApi) {
            return;
        }

        const elements = {
            userName: document.getElementById("user-name"),
            userRole: document.getElementById("user-role"),
            logoutButton: document.getElementById("logout-button"),
            search: document.getElementById("user-search"),
            count: document.getElementById("user-count"),
            loading: document.getElementById("user-loading"),
            list: document.getElementById("user-list"),
            items: document.getElementById("user-items"),
            empty: document.getElementById("user-empty"),
            error: document.getElementById("user-error"),
            errorMessage: document.getElementById("user-error-message"),
            retry: document.getElementById("user-retry"),
            status: document.getElementById("user-status"),
            confirmDialog: document.getElementById("confirm-dialog"),
            confirmMessage: document.getElementById("confirm-message")
        };

        const state = {
            users: [],
            filtered: [],
            query: "",
            busy: false
        };

        if (window.CodeHunterBackground) {
            window.CodeHunterBackground.init("tech-canvas");
        }

        function setStatus(message, statusName) {
            elements.status.textContent = message;
            if (statusName) {
                elements.status.dataset.state = statusName;
            } else {
                delete elements.status.dataset.state;
            }
        }

        function showOnly(element) {
            [elements.loading, elements.list, elements.empty, elements.error]
                .forEach(function (candidate) {
                    candidate.hidden = candidate !== element;
                });
        }

        function updateCount() {
            const total = state.users.length;
            const visible = state.filtered.length;
            elements.count.textContent = state.query
                ? "显示 " + visible + " / " + total + " 个账号"
                : "共 " + total + " 个账号";
        }

        function createUserRow(user) {
            const row = document.createElement("article");
            row.className = "user-row";
            row.setAttribute("role", "listitem");

            const id = document.createElement("span");
            id.className = "problem-id";
            id.textContent = "#" + String(user.id).padStart(3, "0");

            const username = document.createElement("span");
            username.className = "user-row-name";
            username.textContent = user.username;

            const role = document.createElement("span");
            role.className = "user-role-badge";
            role.classList.toggle("is-admin", user.role === "admin");
            role.textContent = user.role === "admin" ? "管理员" : "普通用户";

            const createdAt = document.createElement("span");
            createdAt.className = "user-created-at";
            createdAt.textContent = user.created_at;

            const action = document.createElement("span");
            action.className = "user-row-action";

            if (user.role === "admin") {
                const protectedLabel = document.createElement("span");
                protectedLabel.className = "protected-label";
                protectedLabel.textContent = "受保护";
                action.appendChild(protectedLabel);
            } else {
                const deleteButton = document.createElement("button");
                deleteButton.type = "button";
                deleteButton.className = "danger-button slim";
                deleteButton.textContent = "删除";
                deleteButton.addEventListener("click", function () {
                    deleteUser(user);
                });
                action.appendChild(deleteButton);
            }

            row.appendChild(id);
            row.appendChild(username);
            row.appendChild(role);
            row.appendChild(createdAt);
            row.appendChild(action);
            return row;
        }

        function renderUsers() {
            elements.items.replaceChildren();
            updateCount();

            if (state.filtered.length === 0) {
                showOnly(elements.empty);
                return;
            }

            const fragment = document.createDocumentFragment();
            state.filtered.forEach(function (user) {
                fragment.appendChild(createUserRow(user));
            });
            elements.items.appendChild(fragment);
            showOnly(elements.list);
        }

        function applyFilter() {
            const query = state.query.trim().toLowerCase();
            state.filtered = !query
                ? state.users.slice()
                : state.users.filter(function (user) {
                    return user.username.toLowerCase().includes(query) ||
                           String(user.id).includes(query);
                });
            renderUsers();
        }

        async function loadUsers() {
            showOnly(elements.loading);
            setStatus("", null);

            try {
                const data = await window.OJApi.adminListUsers();
                state.users = data && data.users ? data.users : [];
                applyFilter();
            } catch (error) {
                elements.errorMessage.textContent = mapError(error);
                showOnly(elements.error);
            }
        }

        function askDelete(username) {
            elements.confirmMessage.textContent =
                "确认删除用户 “" + username + "” 吗？该用户的提交记录会一并删除。";
            elements.confirmDialog.showModal();

            return new Promise(function (resolve) {
                elements.confirmDialog.addEventListener("close", function () {
                    resolve(elements.confirmDialog.returnValue === "confirm");
                }, { once: true });
            });
        }

        async function deleteUser(user) {
            if (state.busy) {
                return;
            }

            const confirmed = await askDelete(user.username);
            if (!confirmed) {
                return;
            }

            state.busy = true;
            setStatus("", null);

            try {
                await window.OJApi.adminDeleteUser(user.id);
                setStatus("用户已删除", "success");
                await loadUsers();
            } catch (error) {
                setStatus(mapError(error), "error");
            } finally {
                state.busy = false;
            }
        }

        async function checkAuthentication() {
            try {
                const user = await window.OJApi.me();
                if (user.role !== "admin") {
                    window.location.replace("./problems.html");
                    return false;
                }

                elements.userName.textContent = user.username;
                elements.userRole.textContent = "管理员";
                elements.userRole.hidden = false;
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
        elements.retry.addEventListener("click", loadUsers);
        elements.logoutButton.addEventListener("click", async function () {
            elements.logoutButton.disabled = true;
            try {
                await window.OJApi.logout();
            } finally {
                window.location.replace("./login.html");
            }
        });

        checkAuthentication().then(function (authenticated) {
            if (authenticated) {
                loadUsers();
            }
        });
    }

    document.addEventListener("DOMContentLoaded", initAdminUsersPage);
})();
