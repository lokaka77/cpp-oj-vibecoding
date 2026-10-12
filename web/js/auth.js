(function () {
    "use strict";

    function initTechBackground() {
        const canvas = document.getElementById("tech-canvas");
        if (!canvas || !canvas.getContext) {
            return;
        }

        const context = canvas.getContext("2d");
        const reduceMotion = window.matchMedia(
            "(prefers-reduced-motion: reduce)"
        ).matches;

        let width = 0;
        let height = 0;
        let nodes = [];
        let frameId = 0;

        function buildNodes() {
            const nodeCount = Math.min(
                72,
                Math.max(32, Math.floor((width * height) / 28000))
            );

            nodes = Array.from({ length: nodeCount }, function () {
                return {
                    x: Math.random() * width,
                    y: Math.random() * height,
                    vx: (Math.random() - 0.5) * 0.16,
                    vy: (Math.random() - 0.5) * 0.16,
                    radius: Math.random() * 1.4 + 0.7,
                    accent: Math.random() < 0.12
                };
            });
        }

        function resize() {
            const ratio = Math.min(window.devicePixelRatio || 1, 2);
            width = window.innerWidth;
            height = window.innerHeight;
            canvas.width = Math.floor(width * ratio);
            canvas.height = Math.floor(height * ratio);
            context.setTransform(ratio, 0, 0, ratio, 0, 0);
            buildNodes();
            draw(0);
        }

        function drawGrid() {
            const spacing = 48;
            context.beginPath();
            context.strokeStyle = "rgba(29, 42, 56, 0.42)";
            context.lineWidth = 1;

            for (let x = 0; x <= width; x += spacing) {
                context.moveTo(x, 0);
                context.lineTo(x, height);
            }

            for (let y = 0; y <= height; y += spacing) {
                context.moveTo(0, y);
                context.lineTo(width, y);
            }

            context.stroke();
        }

        function drawConnections() {
            for (let i = 0; i < nodes.length; i += 1) {
                for (let j = i + 1; j < nodes.length; j += 1) {
                    const dx = nodes[i].x - nodes[j].x;
                    const dy = nodes[i].y - nodes[j].y;
                    const distance = Math.sqrt(dx * dx + dy * dy);
                    if (distance > 138) {
                        continue;
                    }

                    const alpha = (1 - distance / 138) * 0.16;
                    context.beginPath();
                    context.strokeStyle =
                        "rgba(38, 208, 206, " + alpha.toFixed(3) + ")";
                    context.lineWidth = 1;
                    context.moveTo(nodes[i].x, nodes[i].y);
                    context.lineTo(nodes[j].x, nodes[j].y);
                    context.stroke();
                }
            }
        }

        function drawNodes() {
            nodes.forEach(function (node) {
                context.beginPath();
                context.fillStyle = node.accent
                    ? "rgba(242, 184, 75, 0.72)"
                    : "rgba(38, 208, 206, 0.62)";
                context.arc(node.x, node.y, node.radius, 0, Math.PI * 2);
                context.fill();
            });
        }

        function updateNodes() {
            nodes.forEach(function (node) {
                node.x += node.vx;
                node.y += node.vy;

                if (node.x < 0 || node.x > width) {
                    node.vx *= -1;
                }
                if (node.y < 0 || node.y > height) {
                    node.vy *= -1;
                }
            });
        }

        function draw() {
            context.clearRect(0, 0, width, height);
            drawGrid();
            drawConnections();
            drawNodes();

            if (!reduceMotion) {
                updateNodes();
                frameId = window.requestAnimationFrame(draw);
            }
        }

        window.addEventListener("resize", function () {
            window.cancelAnimationFrame(frameId);
            resize();
        });

        resize();
    }

    function mapError(error) {
        switch (error.code) {
        case "VALIDATION_ERROR":
            return "请输入完整的账号信息";
        case "USERNAME_EXISTS":
            return "用户名已存在";
        case "INVALID_CREDENTIALS":
            return "用户名或密码错误";
        case "NOT_AUTHENTICATED":
            return "登录状态已失效，请重新登录";
        case "NETWORK_ERROR":
            return "无法连接服务器，请确认后端已启动";
        case "INTERNAL_ERROR":
            return "服务暂时不可用，请稍后重试";
        default:
            return error.message || "请求失败";
        }
    }

    function initAuthPage() {
        const form = document.getElementById("auth-form");
        if (!form || !window.OJApi) {
            return;
        }

        const elements = {
            title: document.getElementById("auth-title"),
            subtitle: document.getElementById("auth-subtitle"),
            tabLogin: document.getElementById("tab-login"),
            tabRegister: document.getElementById("tab-register"),
            username: document.getElementById("username"),
            password: document.getElementById("password"),
            confirmField: document.getElementById("confirm-field"),
            confirmPassword: document.getElementById("confirm-password"),
            status: document.getElementById("form-status"),
            submitButton: document.getElementById("submit-button"),
            submitLabel: document.querySelector("#submit-button .button-label")
        };

        const state = {
            mode: "login",
            busy: false,
            loggedInUser: null
        };

        function setStatus(message, statusName) {
            elements.status.textContent = message;
            if (statusName) {
                elements.status.dataset.state = statusName;
            } else {
                delete elements.status.dataset.state;
            }
        }

        function setBusy(busy) {
            state.busy = busy;
            elements.submitButton.disabled = busy;
            elements.submitButton.classList.toggle("is-loading", busy);
        }

        function setMode(mode) {
            state.mode = mode;
            const isRegister = mode === "register";

            elements.tabLogin.classList.toggle("is-active", !isRegister);
            elements.tabRegister.classList.toggle("is-active", isRegister);
            elements.tabLogin.setAttribute("aria-selected", String(!isRegister));
            elements.tabRegister.setAttribute("aria-selected", String(isRegister));
            elements.title.textContent = isRegister ? "创建新账号" : "登录控制台";
            elements.subtitle.textContent = isRegister
                ? "注册后自动登录"
                : "登录以继续";
            elements.confirmField.hidden = !isRegister;
            elements.confirmPassword.required = isRegister;
            elements.password.autocomplete = isRegister
                ? "new-password"
                : "current-password";
            elements.submitLabel.textContent = isRegister ? "注册并登录" : "登录";

            if (state.loggedInUser) {
                elements.submitLabel.textContent = "进入题目列表";
            }

            setStatus("", null);
            elements.username.removeAttribute("aria-invalid");
            elements.password.removeAttribute("aria-invalid");
            elements.confirmPassword.removeAttribute("aria-invalid");
        }

        function validateForm() {
            let valid = true;

            if (!elements.username.value.trim()) {
                elements.username.setAttribute("aria-invalid", "true");
                valid = false;
            } else {
                elements.username.removeAttribute("aria-invalid");
            }

            if (!elements.password.value) {
                elements.password.setAttribute("aria-invalid", "true");
                valid = false;
            } else {
                elements.password.removeAttribute("aria-invalid");
            }

            if (state.mode === "register") {
                if (!elements.confirmPassword.value ||
                    elements.confirmPassword.value !== elements.password.value) {
                    elements.confirmPassword.setAttribute("aria-invalid", "true");
                    setStatus("两次输入的密码不一致", "error");
                    valid = false;
                } else {
                    elements.confirmPassword.removeAttribute("aria-invalid");
                }
            }

            if (!valid && !elements.status.textContent) {
                setStatus("请填写完整的账号信息", "error");
            }

            return valid;
        }

        async function redirectAfterAuth() {
            setStatus("认证成功，正在进入题目列表", "success");
            window.setTimeout(function () {
                window.location.replace("./problems.html");
            }, 450);
        }

        async function handleSubmit(event) {
            event.preventDefault();
            if (state.busy) {
                return;
            }

            if (state.loggedInUser) {
                await redirectAfterAuth();
                return;
            }

            if (!validateForm()) {
                return;
            }

            const username = elements.username.value.trim();
            const password = elements.password.value;
            setBusy(true);
            setStatus("", null);

            try {
                if (state.mode === "register") {
                    await window.OJApi.register(username, password);
                }

                await window.OJApi.login(username, password);
                await redirectAfterAuth();
            } catch (error) {
                setStatus(mapError(error), "error");
            } finally {
                setBusy(false);
            }
        }

        elements.tabLogin.addEventListener("click", function () {
            setMode("login");
        });
        elements.tabRegister.addEventListener("click", function () {
            setMode("register");
        });
        form.addEventListener("submit", handleSubmit);

        document.querySelectorAll("[data-toggle-password]").forEach(function (button) {
            button.addEventListener("click", function () {
                const input = document.getElementById(
                    button.getAttribute("data-toggle-password")
                );
                if (!input) {
                    return;
                }

                const visible = input.type === "text";
                input.type = visible ? "password" : "text";
                button.setAttribute("aria-pressed", String(!visible));
                button.setAttribute(
                    "aria-label",
                    visible ? "显示密码" : "隐藏密码"
                );
            });
        });

        [elements.tabLogin, elements.tabRegister].forEach(function (tab) {
            tab.addEventListener("keydown", function (event) {
                if (event.key === "ArrowLeft" || event.key === "ArrowRight") {
                    event.preventDefault();
                    const next = event.key === "ArrowRight"
                        ? elements.tabRegister
                        : elements.tabLogin;
                    next.focus();
                    next.click();
                }
            });
        });

        initTechBackground();

        window.OJApi.me().then(function (user) {
            state.loggedInUser = user;
            elements.username.value = user.username;
            elements.submitLabel.textContent = "进入题目列表";
            setStatus("当前已登录：" + user.username, "success");
        }).catch(function () {
            state.loggedInUser = null;
        });
    }

    document.addEventListener("DOMContentLoaded", initAuthPage);
})();
