#include "admin_service.h"

#include "oj/models.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace {

void send_json(httplib::Response& response,
               int status,
               const nlohmann::json& body) {
    response.status = status;
    response.set_content(body.dump(), "application/json");
}

std::string cookie_value(const httplib::Request& request,
                         const std::string& name) {
    std::string header = request.get_header_value("Cookie");
    if (header.empty()) {
        return {};
    }

    std::size_t position = 0;
    while (position < header.size()) {
        std::size_t end = header.find(';', position);
        if (end == std::string::npos) {
            end = header.size();
        }

        std::string part = header.substr(position, end - position);
        std::size_t equals = part.find('=');
        if (equals != std::string::npos &&
            part.substr(0, equals) == name) {
            return part.substr(equals + 1);
        }

        position = end + 1;
    }

    return {};
}

bool require_admin(const httplib::Request& request,
                   httplib::Response& response,
                   Database& database,
                   SessionManager& sessions) {
    std::uint64_t user_id = 0;
    if (!sessions.get(cookie_value(request, "session_id"), user_id)) {
        send_json(response, 401, {{"error", "not authenticated"}});
        return false;
    }

    AuthUser user;
    if (!database.find_user_by_id(user_id, user)) {
        send_json(response, 401, {{"error", "user not found"}});
        return false;
    }

    if (user.role != "admin") {
        send_json(response, 403, {{"error", "admin permission required"}});
        return false;
    }

    return true;
}

bool parse_json_body(const httplib::Request& request,
                     nlohmann::json& body) {
    try {
        body = nlohmann::json::parse(request.body);
        return body.is_object();
    } catch (const std::exception&) {
        return false;
    }
}

}  // namespace

void register_admin_problem_routes(httplib::Server& server,
                                   Database& database,
                                   SessionManager& sessions) {
    server.Get("/api/admin/problems",
               [&](const httplib::Request& request,
                   httplib::Response& response) {
        if (!require_admin(request, response, database, sessions)) {
            return;
        }

        std::vector<ProblemSummary> problems;
        if (!database.list_problems(problems)) {
            send_json(response, 500, {{"error", "failed to load problems"}});
            return;
        }

        nlohmann::json items = nlohmann::json::array();
        for (const ProblemSummary& problem : problems) {
            items.push_back({{"id", problem.id}, {"title", problem.title}});
        }
        send_json(response, 200, {{"problems", items}});
    });

    server.Post("/api/admin/problems",
                [&](const httplib::Request& request,
                    httplib::Response& response) {
        if (!require_admin(request, response, database, sessions)) {
            return;
        }

        nlohmann::json body;
        if (!parse_json_body(request, body)) {
            send_json(response, 400, {{"error", "invalid JSON body"}});
            return;
        }

        std::string title = body.value("title", "");
        std::string description = body.value("description", "");
        if (title.empty()) {
            send_json(response, 400, {{"error", "title is required"}});
            return;
        }

        std::uint64_t problem_id = 0;
        if (!database.insert_problem(title, description, problem_id)) {
            send_json(response, 500, {{"error", "failed to create problem"}});
            return;
        }

        send_json(response, 201,
                  {{"id", problem_id}, {"title", title}});
    });

    server.Put(R"(/api/admin/problems/(\d+))",
               [&](const httplib::Request& request,
                   httplib::Response& response) {
        if (!require_admin(request, response, database, sessions)) {
            return;
        }

        nlohmann::json body;
        if (!parse_json_body(request, body)) {
            send_json(response, 400, {{"error", "invalid JSON body"}});
            return;
        }

        std::string title = body.value("title", "");
        std::string description = body.value("description", "");
        if (title.empty()) {
            send_json(response, 400, {{"error", "title is required"}});
            return;
        }

        std::uint64_t problem_id = std::stoull(request.matches[1].str());
        Problem existing;
        if (!database.find_problem_by_id(problem_id, existing)) {
            send_json(response, 404, {{"error", "problem not found"}});
            return;
        }

        if (!database.update_problem(problem_id, title, description)) {
            send_json(response, 500, {{"error", "failed to update problem"}});
            return;
        }

        send_json(response, 200, {{"status", "ok"}});
    });

    server.Delete(R"(/api/admin/problems/(\d+))",
                  [&](const httplib::Request& request,
                      httplib::Response& response) {
        if (!require_admin(request, response, database, sessions)) {
            return;
        }

        std::uint64_t problem_id = std::stoull(request.matches[1].str());
        Problem existing;
        if (!database.find_problem_by_id(problem_id, existing)) {
            send_json(response, 404, {{"error", "problem not found"}});
            return;
        }

        if (!database.delete_problem(problem_id)) {
            send_json(response, 500, {{"error", "failed to delete problem"}});
            return;
        }

        send_json(response, 200, {{"status", "ok"}});
    });
}
