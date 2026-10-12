(function () {
    "use strict";

    const DEFAULT_CODE = [
        "#include <iostream>",
        "using namespace std;",
        "",
        "int main() {",
        "    ",
        "    return 0;",
        "}",
        ""
    ].join("\n");

    function mapError(error) {
        switch (error.code) {
        case "PROBLEM_NOT_FOUND":
            return "题目不存在或已被删除";
        case "NETWORK_ERROR":
            return "无法连接服务器，请确认后端已启动";
        case "RATE_LIMITED":
            return "提交过于频繁，请稍后重试";
        case "JUDGE_TIMEOUT":
            return "判题服务超时，请稍后重试";
        case "NOT_AUTHENTICATED":
            return "登录状态已失效，请重新登录";
        case "INTERNAL_ERROR":
            return "服务暂时不可用，请稍后重试";
        default:
            return error.message || "请求失败";
        }
    }

    function initProblemPage() {
        if (!window.OJApi) {
            return;
        }

        const elements = {
            userName: document.getElementById("user-name"),
            userRole: document.getElementById("user-role"),
            adminNav: document.getElementById("admin-nav-link"),
            logoutButton: document.getElementById("logout-button"),
            meta: document.getElementById("problem-meta"),
            loading: document.getElementById("problem-loading"),
            error: document.getElementById("problem-error"),
            errorMessage: document.getElementById("problem-error-message"),
            retry: document.getElementById("problem-retry"),
            content: document.getElementById("problem-content"),
            title: document.getElementById("problem-title"),
            description: document.getElementById("problem-description"),
            samples: document.getElementById("sample-list"),
            editor: document.getElementById("code-editor"),
            lineNumbers: document.getElementById("line-numbers"),
            runButton: document.getElementById("run-button"),
            submitButton: document.getElementById("submit-button"),
            resultPanel: document.getElementById("result-panel"),
            resultVerdict: document.getElementById("result-verdict"),
            resultMessage: document.getElementById("result-message")
        };

        const params = new URLSearchParams(window.location.search);
        const problemId = params.get("id");

        const state = {
            problemId: problemId,
            busy: false,
            draftKey: problemId ? "codehunter:problem:" + problemId : ""
        };

        if (window.CodeHunterBackground) {
            window.CodeHunterBackground.init("tech-canvas");
        }

        function showOnly(element) {
            [elements.loading, elements.error, elements.content]
                .forEach(function (candidate) {
                    candidate.hidden = candidate !== element;
                });
        }

        function setBusy(busy) {
            state.busy = busy;
            elements.runButton.disabled = busy;
            elements.submitButton.disabled = busy;
            elements.runButton.classList.toggle("is-loading", busy);
            elements.submitButton.classList.toggle("is-loading", busy);
        }

        function showResult(verdict, message, stateName) {
            elements.resultPanel.hidden = false;
            elements.resultPanel.dataset.state = stateName || "";
            elements.resultVerdict.textContent = verdict;
            elements.resultMessage.textContent = message;
        }

        function clearResult() {
            elements.resultPanel.hidden = true;
            elements.resultVerdict.textContent = "";
            elements.resultMessage.textContent = "";
        }

        function updateLineNumbers() {
            const lineCount = elements.editor.value.split("\n").length;
            const lines = [];
            for (let index = 1; index <= lineCount; index += 1) {
                lines.push(index);
            }
            elements.lineNumbers.textContent = lines.join("\n");
        }

        function saveDraft() {
            if (!state.draftKey) {
                return;
            }
            try {
                window.localStorage.setItem(state.draftKey, elements.editor.value);
            } catch (error) {
                /* localStorage can be unavailable in private modes. */
            }
        }

        function loadDraft() {
            if (!state.draftKey) {
                return DEFAULT_CODE;
            }
            try {
                return window.localStorage.getItem(state.draftKey) || DEFAULT_CODE;
            } catch (error) {
                return DEFAULT_CODE;
            }
        }

        function createSampleBlock(label, content) {
            const block = document.createElement("div");
            block.className = "sample-block";

            const heading = document.createElement("span");
            heading.className = "sample-label";
            heading.textContent = label;

            const pre = document.createElement("pre");
            pre.textContent = content;

            block.appendChild(heading);
            block.appendChild(pre);
            return block;
        }

        function renderSamples(samples) {
            elements.samples.replaceChildren();

            if (!samples || samples.length === 0) {
                const empty = document.createElement("p");
                empty.className = "sample-empty";
                empty.textContent = "暂无样例";
                elements.samples.appendChild(empty);
                return;
            }

            samples.forEach(function (sample, index) {
                const item = document.createElement("article");
                item.className = "sample-item";

                const header = document.createElement("h3");
                header.textContent = "样例 " + (index + 1);
                item.appendChild(header);
                item.appendChild(createSampleBlock("输入", sample.input));
                item.appendChild(
                    createSampleBlock("期望输出", sample.expected_output)
                );
                elements.samples.appendChild(item);
            });
        }

        function renderProblem(problem) {
            elements.title.textContent = problem.title;
            elements.description.textContent = problem.description;
            elements.meta.textContent = "题目 #" + String(problem.id).padStart(3, "0");
            renderSamples(problem.samples);
            document.title = "CodeHunter - " + problem.title;
        }

        async function loadProblem() {
            if (!state.problemId) {
                elements.errorMessage.textContent = "缺少题目 ID";
                showOnly(elements.error);
                return;
            }

            showOnly(elements.loading);
            clearResult();

            try {
                const problem = await window.OJApi.getProblem(state.problemId);
                renderProblem(problem);
                showOnly(elements.content);
            } catch (error) {
                elements.errorMessage.textContent = mapError(error);
                showOnly(elements.error);
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

        async function runSample() {
            if (state.busy) {
                return;
            }

            if (!elements.editor.value.trim()) {
                showResult("CE", "代码不能为空", "error");
                return;
            }

            setBusy(true);
            clearResult();

            try {
                const result = await window.OJApi.runSample(
                    state.problemId,
                    elements.editor.value
                );
                showResult(
                    result.passed ? "PASS" : "FAIL",
                    result.message,
                    result.passed ? "success" : "error"
                );
            } catch (error) {
                showResult("ERROR", mapError(error), "error");
            } finally {
                setBusy(false);
            }
        }

        async function submitCode() {
            if (state.busy) {
                return;
            }

            if (!elements.editor.value.trim()) {
                showResult("CE", "代码不能为空", "error");
                return;
            }

            setBusy(true);
            clearResult();

            try {
                const result = await window.OJApi.submit(
                    state.problemId,
                    elements.editor.value
                );
                const success = result.verdict === "AC";
                const messages = {
                    AC: "全部测试点通过",
                    WA: "输出与期望不一致",
                    TLE: "运行超时",
                    RE: "运行时错误或内存超限",
                    CE: "编译错误"
                };
                showResult(
                    result.verdict,
                    messages[result.verdict] || "判题完成",
                    success ? "success" : "error"
                );
            } catch (error) {
                const message = error.code === "RATE_LIMITED" &&
                    error.details && error.details.retry_after_ms
                    ? "提交过于频繁，请等待 " +
                      Math.ceil(error.details.retry_after_ms / 1000) +
                      " 秒后重试"
                    : mapError(error);
                showResult("ERROR", message, "error");
            } finally {
                setBusy(false);
            }
        }

        elements.editor.value = loadDraft();
        updateLineNumbers();

        elements.editor.addEventListener("input", function () {
            updateLineNumbers();
            saveDraft();
        });

        elements.editor.addEventListener("scroll", function () {
            elements.lineNumbers.scrollTop = elements.editor.scrollTop;
        });

        elements.editor.addEventListener("keydown", function (event) {
            if (event.key !== "Tab") {
                return;
            }

            event.preventDefault();
            const start = elements.editor.selectionStart;
            const end = elements.editor.selectionEnd;
            const value = elements.editor.value;
            elements.editor.value =
                value.substring(0, start) + "    " + value.substring(end);
            elements.editor.selectionStart = start + 4;
            elements.editor.selectionEnd = start + 4;
            updateLineNumbers();
            saveDraft();
        });

        elements.runButton.addEventListener("click", runSample);
        elements.submitButton.addEventListener("click", submitCode);
        elements.retry.addEventListener("click", loadProblem);

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
                loadProblem();
            }
        });
    }

    document.addEventListener("DOMContentLoaded", initProblemPage);
})();
