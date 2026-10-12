(function () {
    "use strict";

    async function request(path, options) {
        const config = options || {};
        const headers = Object.assign(
            { "Content-Type": "application/json" },
            config.headers || {}
        );

        let response;
        try {
            response = await fetch(path, {
                method: config.method || "GET",
                headers: headers,
                credentials: "same-origin",
                body: config.body
            });
        } catch (error) {
            const networkError = new Error("无法连接服务器");
            networkError.code = "NETWORK_ERROR";
            throw networkError;
        }

        let data = null;
        try {
            data = await response.json();
        } catch (error) {
            data = null;
        }

        if (!response.ok) {
            const details = data && data.error ? data.error : {};
            const requestError = new Error(details.message || "请求失败");
            requestError.code = details.code || "REQUEST_FAILED";
            requestError.status = response.status;
            requestError.details = details.details || null;
            throw requestError;
        }

        return data;
    }

    window.OJApi = {
        request: request,
        register: function (username, password) {
            return request("/api/auth/register", {
                method: "POST",
                body: JSON.stringify({
                    username: username,
                    password: password
                })
            });
        },
        login: function (username, password) {
            return request("/api/auth/login", {
                method: "POST",
                body: JSON.stringify({
                    username: username,
                    password: password
                })
            });
        },
        logout: function () {
            return request("/api/auth/logout", { method: "POST" });
        },
        me: function () {
            return request("/api/auth/me");
        },
        listProblems: function () {
            return request("/api/problems");
        },
        listSolvedProblems: function () {
            return request("/api/problems/solved");
        },
        getProblem: function (problemId) {
            return request("/api/problems/" + encodeURIComponent(problemId));
        },
        runSample: function (problemId, code) {
            return request(
                "/api/problems/" + encodeURIComponent(problemId) + "/run",
                {
                    method: "POST",
                    body: JSON.stringify({ code: code })
                }
            );
        },
        submit: function (problemId, code) {
            return request(
                "/api/problems/" + encodeURIComponent(problemId) + "/submit",
                {
                    method: "POST",
                    body: JSON.stringify({ code: code })
                }
            );
        },
        adminListProblems: function () {
            return request("/api/admin/problems");
        },
        adminCreateProblem: function (title, description) {
            return request("/api/admin/problems", {
                method: "POST",
                body: JSON.stringify({
                    title: title,
                    description: description
                })
            });
        },
        adminUpdateProblem: function (problemId, title, description) {
            return request(
                "/api/admin/problems/" + encodeURIComponent(problemId),
                {
                    method: "PUT",
                    body: JSON.stringify({
                        title: title,
                        description: description
                    })
                }
            );
        },
        adminDeleteProblem: function (problemId) {
            return request(
                "/api/admin/problems/" + encodeURIComponent(problemId),
                { method: "DELETE" }
            );
        },
        adminImportTestCases: function (problemId, testCases) {
            return request(
                "/api/admin/problems/" + encodeURIComponent(problemId) +
                    "/testcases",
                {
                    method: "POST",
                    body: JSON.stringify({ test_cases: testCases })
                }
            );
        },
        adminListUsers: function () {
            return request("/api/admin/users");
        },
        adminDeleteUser: function (userId) {
            return request(
                "/api/admin/users/" + encodeURIComponent(userId),
                { method: "DELETE" }
            );
        }
    };
})();
