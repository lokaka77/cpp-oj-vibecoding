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
        }
    };
})();
