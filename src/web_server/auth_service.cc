#include "auth_service.h"

#include "logging.h"
#include "password.h"

#include <nlohmann/json.hpp>

#include <string>

namespace {

void send_json(httplib::Response& response,
               int status,
               const nlohmann::json& body) {
    response.status = status;
    response.set_content(body.dump(), "application/json");
}

bool parse_json_body(const httplib::Request& request,
                     nlohmann::json& body) {
    try {
        if (request.body.empty()) {
            log_warning("auth request body is empty");
            return false;
        }
        body = nlohmann::json::parse(request.body);
        return body.is_object();
    } catch (const std::exception& error) {
        log_warning(std::string("JSON parse failed: ") + error.what());
        return false;
    }
}

std::string cookie_value(const httplib::Request& request,
                         const std::string& name) {
    std::string cookie_header = request.get_header_value("Cookie");
    if (cookie_header.empty()) {
        return {};
    }

    std::size_t position = 0;
    while (position < cookie_header.size()) {
        std::size_t end = cookie_header.find(';', position);
        if (end == std::string::npos) {
            end = cookie_header.size();
        }

        std::string part = cookie_header.substr(position, end - position);
        std::size_t equals = part.find('=');
        if (equals != std::string::npos) {
            std::string key = part.substr(0, equals);
            if (key == name) {
                return part.substr(equals + 1);
            }
        }

        position = end + 1;
    }

    return {};
}

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
            send_json(response, 400, {{"error", "invalid JSON body"}});
            return;
        }

        std::string username = body.value("username", "");
        std::string password = body.value("password", "");
        if (username.empty() || password.empty()) {
            send_json(response, 400, {{"error", "username and password are required"}});
            return;
        }

        AuthUser existing;
        if (database.find_user_by_username(username, existing)) {
            send_json(response, 409, {{"error", "username already exists"}});
            return;
        }

        std::string hash = hash_password(password);
        if (hash.empty() ||
            !database.insert_user(username, hash, "user")) {
            send_json(response, 500, {{"error", "failed to create user"}});
            return;
        }

        AuthUser created;
        if (!database.find_user_by_username(username, created)) {
            send_json(response, 500, {{"error", "failed to load created user"}});
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
            send_json(response, 400, {{"error", "invalid JSON body"}});
            return;
        }

        std::string username = body.value("username", "");
        std::string password = body.value("password", "");
        AuthUser user;
        if (!database.find_user_by_username(username, user) ||
            !verify_password(user.password_hash, password)) {
            send_json(response, 401, {{"error", "invalid username or password"}});
            return;
        }

        std::string token = sessions.create(user.id);
        if (token.empty()) {
            send_json(response, 500, {{"error", "failed to create session"}});
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
        std::uint64_t user_id = 0;
        if (!sessions.get(cookie_value(request, "session_id"), user_id)) {
            send_json(response, 401, {{"error", "not authenticated"}});
            return;
        }

        AuthUser user;
        if (!database.find_user_by_id(user_id, user)) {
            send_json(response, 401, {{"error", "user not found"}});
            return;
        }

        send_json(response, 200,
                  {{"id", user.id},
                   {"username", user.username},
                   {"role", user.role}});
    });
}
