#include "auth_service.h"

#include "auth_middleware.h"
#include "http_utils.h"
#include "password.h"

#include <nlohmann/json.hpp>

namespace {

void set_session_cookie(httplib::Response& response,
                        const std::string& token) {
    response.set_header("Set-Cookie",
                        "session_id=" + token +
                            "; Path=/; HttpOnly; SameSite=Lax");
}

void clear_session_cookie(httplib::Response& response) {
    response.set_header("Set-Cookie",
                        "session_id=; Path=/; HttpOnly; SameSite=Lax; Max-Age=0");
}

}  // namespace

void register_auth_routes(httplib::Server& server,
                          Database& database,
                          SessionManager& sessions) {
    server.Post("/api/auth/register", [&](const httplib::Request& request,
                                          httplib::Response& response) {
        nlohmann::json body;
        if (!parse_json_body(request, body)) {
            send_error(response, 400, "INVALID_JSON", "invalid JSON body");
            return;
        }

        std::string username = body.value("username", "");
        std::string password = body.value("password", "");
        if (username.empty() || password.empty()) {
            send_error(response, 400, "VALIDATION_ERROR",
                       "username and password are required");
            return;
        }

        AuthUser existing;
        if (database.find_user_by_username(username, existing)) {
            send_error(response, 409, "USERNAME_EXISTS",
                       "username already exists");
            return;
        }

        std::string hash = hash_password(password);
        if (hash.empty() ||
            !database.insert_user(username, hash, "user")) {
            send_error(response, 500, "INTERNAL_ERROR",
                       "failed to create user");
            return;
        }

        AuthUser created;
        if (!database.find_user_by_username(username, created)) {
            send_error(response, 500, "INTERNAL_ERROR",
                       "failed to load created user");
            return;
        }

        send_json(response, 201,
                  {{"id", created.id},
                   {"username", created.username},
                   {"role", created.role}});
    });

    server.Post("/api/auth/login", [&](const httplib::Request& request,
                                       httplib::Response& response) {
        nlohmann::json body;
        if (!parse_json_body(request, body)) {
            send_error(response, 400, "INVALID_JSON", "invalid JSON body");
            return;
        }

        std::string username = body.value("username", "");
        std::string password = body.value("password", "");
        AuthUser user;
        if (!database.find_user_by_username(username, user) ||
            !verify_password(user.password_hash, password)) {
            send_error(response, 401, "INVALID_CREDENTIALS",
                       "invalid username or password");
            return;
        }

        std::string token = sessions.create(user.id);
        if (token.empty()) {
            send_error(response, 500, "INTERNAL_ERROR",
                       "failed to create session");
            return;
        }

        set_session_cookie(response, token);
        send_json(response, 200,
                  {{"id", user.id},
                   {"username", user.username},
                   {"role", user.role}});
    });

    server.Post("/api/auth/logout", [&](const httplib::Request& request,
                                        httplib::Response& response) {
        std::string token = cookie_value(request, "session_id");
        sessions.remove(token);
        clear_session_cookie(response);
        send_json(response, 200, {{"status", "ok"}});
    });

    server.Get("/api/auth/me", [&](const httplib::Request& request,
                                   httplib::Response& response) {
        AuthUser user;
        if (!require_authenticated(request, response, database, sessions,
                                   user)) {
            return;
        }

        send_json(response, 200,
                  {{"id", user.id},
                   {"username", user.username},
                   {"role", user.role}});
    });
}
