(function () {
    "use strict";

    function mapError(error) {
        switch (error.code) {
        case "NOT_AUTHENTICATED":
            return "登录状态已失效，请重新登录";
        case "ADMIN_REQUIRED":
            return "当前账号没有管理员权限";
        case "PROBLEM_NOT_FOUND":
            return "题目不存在或已被删除";
        case "VALIDATION_ERROR":
            return "请检查表单内容";
        case "NETWORK_ERROR":
            return "无法连接服务器，请确认后端已启动";
        case "INTERNAL_ERROR":
            return "服务暂时不可用，请稍后重试";
        default:
            return error.message || "请求失败";
        }
    }

    function initAdminPage() {
        if (!window.OJApi) {
            return;
        }

        const elements = {
            userName: document.getElementById("user-name"),
            userRole: document.getElementById("user-role"),
            logoutButton: document.getElementById("logout-button"),
            newProblemButton: document.getElementById("new-problem-button"),
            listLoading: document.getElementById("admin-list-loading"),
            list: document.getElementById("admin-problem-list"),
            listEmpty: document.getElementById("admin-list-empty"),
            count: document.getElementById("admin-problem-count"),
            editorEmpty: document.getElementById("admin-editor-empty"),
            form: document.getElementById("problem-form"),
            problemId: document.getElementById("admin-problem-id"),
            title: document.getElementById("admin-title"),
            description: document.getElementById("admin-description"),
            saveButton: document.getElementById("save-problem-button"),
            deleteButton: document.getElementById("delete-problem-button"),
            testcaseEditor: document.getElementById("testcase-editor"),
            testcaseList: document.getElementById("testcase-list"),
            addTestcaseButton: document.getElementById("add-testcase-button"),
            importButton: document.getElementById("import-testcases-button"),
            status: document.getElementById("admin-status"),
            confirmDialog: document.getElementById("confirm-dialog"),
            confirmMessage: document.getElementById("confirm-message")
        };

        const state = {
            problems: [],
            selectedId: null,
            mode: "empty",
            testCases: [],
            busy: false
        };
        const initialProblemId = new URLSearchParams(
            window.location.search
        ).get("id");

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

        function setBusy(busy) {
            state.busy = busy;
            elements.saveButton.disabled = busy;
            elements.deleteButton.disabled = busy;
            elements.importButton.disabled = busy;
            elements.addTestcaseButton.disabled =
                busy || state.testCases.length >= 5;
            elements.newProblemButton.disabled = busy;
        }

        function showEditor() {
            elements.editorEmpty.hidden = true;
            elements.form.hidden = false;
            elements.testcaseEditor.hidden = state.mode !== "edit";
        }

        function showEmptyEditor() {
            state.mode = "empty";
            state.selectedId = null;
            elements.editorEmpty.hidden = false;
            elements.form.hidden = true;
            elements.testcaseEditor.hidden = true;
            elements.title.value = "";
            elements.description.value = "";
            elements.problemId.textContent = "";
            setStatus("", null);
        }

        function renderProblemList() {
            elements.list.replaceChildren();
            elements.count.textContent = String(state.problems.length);

            if (state.problems.length === 0) {
                elements.list.hidden = true;
                elements.listEmpty.hidden = false;
                return;
            }

            const fragment = document.createDocumentFragment();
            state.problems.forEach(function (problem) {
                const button = document.createElement("button");
                button.type = "button";
                button.className = "admin-problem-item";
                button.setAttribute("role", "listitem");
                button.classList.toggle(
                    "is-selected",
                    String(problem.id) === String(state.selectedId)
                );

                const id = document.createElement("span");
                id.className = "problem-id";
                id.textContent = "#" + String(problem.id).padStart(3, "0");

                const title = document.createElement("span");
                title.className = "admin-problem-title";
                title.textContent = problem.title;

                button.appendChild(id);
                button.appendChild(title);
                button.addEventListener("click", function () {
                    selectProblem(problem.id);
                });
                fragment.appendChild(button);
            });

            elements.list.appendChild(fragment);
            elements.list.hidden = false;
            elements.listEmpty.hidden = true;
        }

        async function loadProblemList() {
            elements.listLoading.hidden = false;
            elements.list.hidden = true;
            elements.listEmpty.hidden = true;

            try {
                const data = await window.OJApi.adminListProblems();
                state.problems = data && data.problems ? data.problems : [];
                renderProblemList();
            } catch (error) {
                setStatus(mapError(error), "error");
            } finally {
                elements.listLoading.hidden = true;
            }
        }

        function createTestCaseRow(testCase, index) {
            const row = document.createElement("article");
            row.className = "testcase-row";

            const header = document.createElement("div");
            header.className = "testcase-row-header";

            const title = document.createElement("h3");
            title.textContent = "用例 " + (index + 1);

            const actions = document.createElement("div");
            actions.className = "testcase-row-actions";

            const sampleLabel = document.createElement("label");
            sampleLabel.className = "checkbox-label";
            const sampleInput = document.createElement("input");
            sampleInput.type = "checkbox";
            sampleInput.checked = Boolean(testCase.is_sample);
            sampleInput.addEventListener("change", function () {
                state.testCases[index].is_sample = sampleInput.checked;
            });
            sampleLabel.appendChild(sampleInput);
            sampleLabel.appendChild(document.createTextNode("样例"));

            const removeButton = document.createElement("button");
            removeButton.type = "button";
            removeButton.className = "icon-text-button";
            removeButton.textContent = "删除";
            removeButton.disabled = state.testCases.length <= 1;
            removeButton.addEventListener("click", function () {
                state.testCases.splice(index, 1);
                renderTestCases();
            });

            actions.appendChild(sampleLabel);
            actions.appendChild(removeButton);
            header.appendChild(title);
            header.appendChild(actions);

            const inputLabel = document.createElement("label");
            inputLabel.className = "testcase-field";
            inputLabel.textContent = "输入";
            const input = document.createElement("textarea");
            input.rows = 3;
            input.value = testCase.input || "";
            input.setAttribute("aria-label", "用例 " + (index + 1) + " 输入");
            input.addEventListener("input", function () {
                state.testCases[index].input = input.value;
            });
            inputLabel.appendChild(input);

            const outputLabel = document.createElement("label");
            outputLabel.className = "testcase-field";
            outputLabel.textContent = "期望输出";
            const output = document.createElement("textarea");
            output.rows = 3;
            output.value = testCase.expected_output || "";
            output.setAttribute("aria-label", "用例 " + (index + 1) + " 期望输出");
            output.addEventListener("input", function () {
                state.testCases[index].expected_output = output.value;
            });
            outputLabel.appendChild(output);

            row.appendChild(header);
            row.appendChild(inputLabel);
            row.appendChild(outputLabel);
            return row;
        }

        function renderTestCases() {
            elements.testcaseList.replaceChildren();

            state.testCases.forEach(function (testCase, index) {
                elements.testcaseList.appendChild(
                    createTestCaseRow(testCase, index)
                );
            });

            elements.addTestcaseButton.disabled = state.testCases.length >= 5;
        }

        function resetTestCases() {
            state.testCases = [
                { input: "", expected_output: "", is_sample: true }
            ];
            renderTestCases();
        }

        async function selectProblem(problemId) {
            setStatus("", null);
            state.selectedId = problemId;
            state.mode = "edit";

            try {
                const problem = await window.OJApi.getProblem(problemId);
                elements.title.value = problem.title;
                elements.description.value = problem.description;
                elements.problemId.textContent =
                    "题目 #" + String(problem.id).padStart(3, "0");
                elements.deleteButton.hidden = false;
                resetTestCases();
                showEditor();
                renderProblemList();
            } catch (error) {
                setStatus(mapError(error), "error");
            }
        }

        function startNewProblem() {
            state.mode = "create";
            state.selectedId = null;
            elements.title.value = "";
            elements.description.value = "";
            elements.problemId.textContent = "新题目";
            elements.deleteButton.hidden = true;
            elements.testcaseEditor.hidden = true;
            elements.editorEmpty.hidden = true;
            elements.form.hidden = false;
            setStatus("", null);
            elements.title.focus();
        }

        async function saveProblem(event) {
            event.preventDefault();
            if (state.busy) {
                return;
            }

            const title = elements.title.value.trim();
            const description = elements.description.value.trim();
            if (!title || !description) {
                setStatus("题目标题和描述不能为空", "error");
                return;
            }

            setBusy(true);
            setStatus("", null);

            try {
                if (state.mode === "create") {
                    const result = await window.OJApi.adminCreateProblem(
                        title,
                        description
                    );
                    state.selectedId = result.id;
                    state.mode = "edit";
                    elements.deleteButton.hidden = false;
                    elements.problemId.textContent =
                        "题目 #" + String(result.id).padStart(3, "0");
                    resetTestCases();
                    setStatus("题目创建成功", "success");
                } else if (state.mode === "edit" && state.selectedId) {
                    await window.OJApi.adminUpdateProblem(
                        state.selectedId,
                        title,
                        description
                    );
                    setStatus("题目保存成功", "success");
                }

                elements.testcaseEditor.hidden = state.mode !== "edit";
                await loadProblemList();
            } catch (error) {
                setStatus(mapError(error), "error");
            } finally {
                setBusy(false);
            }
        }

        function askDelete() {
            if (!state.selectedId) {
                return Promise.resolve(false);
            }

            elements.confirmMessage.textContent =
                "删除后无法恢复，确定删除该题目吗？";
            elements.confirmDialog.showModal();

            return new Promise(function (resolve) {
                elements.confirmDialog.addEventListener("close", function () {
                    resolve(elements.confirmDialog.returnValue === "confirm");
                }, { once: true });
            });
        }

        async function deleteProblem() {
            const confirmed = await askDelete();
            if (!confirmed || !state.selectedId || state.busy) {
                return;
            }

            setBusy(true);
            setStatus("", null);

            try {
                await window.OJApi.adminDeleteProblem(state.selectedId);
                showEmptyEditor();
                state.testCases = [];
                renderTestCases();
                setStatus("题目已删除", "success");
                await loadProblemList();
            } catch (error) {
                setStatus(mapError(error), "error");
            } finally {
                setBusy(false);
            }
        }

        async function importTestCases() {
            if (!state.selectedId || state.busy) {
                return;
            }

            if (state.testCases.length < 1 || state.testCases.length > 5) {
                setStatus("测试用例数量必须在 1 到 5 组之间", "error");
                return;
            }

            const hasSample = state.testCases.some(function (testCase) {
                return testCase.is_sample;
            });
            if (!hasSample) {
                setStatus("至少需要一组样例测试点", "error");
                return;
            }

            setBusy(true);
            setStatus("", null);

            try {
                const result = await window.OJApi.adminImportTestCases(
                    state.selectedId,
                    state.testCases
                );
                setStatus(
                    "已导入 " + result.count + " 组测试用例",
                    "success"
                );
            } catch (error) {
                setStatus(mapError(error), "error");
            } finally {
                setBusy(false);
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

        elements.newProblemButton.addEventListener("click", startNewProblem);
        elements.form.addEventListener("submit", saveProblem);
        elements.deleteButton.addEventListener("click", deleteProblem);
        elements.addTestcaseButton.addEventListener("click", function () {
            if (state.testCases.length >= 5) {
                return;
            }
            state.testCases.push({
                input: "",
                expected_output: "",
                is_sample: false
            });
            renderTestCases();
        });
        elements.importButton.addEventListener("click", importTestCases);
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
                loadProblemList().then(function () {
                    if (initialProblemId) {
                        selectProblem(initialProblemId);
                    }
                });
            }
        });
    }

    document.addEventListener("DOMContentLoaded", initAdminPage);
})();
