#include "admin_service.h"

#include "auth_middleware.h"
#include "http_utils.h"
#include "oj/models.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

bool require_admin_request(const httplib::Request& request,
                           httplib::Response& response,
                           Database& database,
                           SessionManager& sessions) {
    AuthUser admin;
    return require_admin(request, response, database, sessions, admin);
}

}  // namespace

void register_admin_problem_routes(httplib::Server& server,
                                   Database& database,
                                   SessionManager& sessions) {
    server.Get("/api/admin/problems",
               [&](const httplib::Request& request,
                   httplib::Response& response) {
        if (!require_admin_request(request, response, database, sessions)) {
            return;
        }

        std::vector<ProblemSummary> problems;
        if (!database.list_problems(problems)) {
            send_error(response, 500, "INTERNAL_ERROR",
                       "failed to load problems");
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
        if (!require_admin_request(request, response, database, sessions)) {
            return;
        }

        nlohmann::json body;
        if (!parse_json_body(request, body)) {
            send_error(response, 400, "INVALID_JSON", "invalid JSON body");
            return;
        }

        std::string title = body.value("title", "");
        std::string description = body.value("description", "");
        if (title.empty()) {
            send_error(response, 400, "VALIDATION_ERROR",
                       "title is required");
            return;
        }

        std::uint64_t problem_id = 0;
        if (!database.insert_problem(title, description, problem_id)) {
            send_error(response, 500, "INTERNAL_ERROR",
                       "failed to create problem");
            return;
        }

        send_json(response, 201,
                  {{"id", problem_id}, {"title", title}});
    });

    server.Put(R"(/api/admin/problems/(\d+))",
               [&](const httplib::Request& request,
                   httplib::Response& response) {
        if (!require_admin_request(request, response, database, sessions)) {
            return;
        }

        nlohmann::json body;
        if (!parse_json_body(request, body)) {
            send_error(response, 400, "INVALID_JSON", "invalid JSON body");
            return;
        }

        std::string title = body.value("title", "");
        std::string description = body.value("description", "");
        if (title.empty()) {
            send_error(response, 400, "VALIDATION_ERROR",
                       "title is required");
            return;
        }

        std::uint64_t problem_id = std::stoull(request.matches[1].str());
        Problem existing;
        if (!database.find_problem_by_id(problem_id, existing)) {
            send_error(response, 404, "PROBLEM_NOT_FOUND",
                       "problem not found");
            return;
        }

        if (!database.update_problem(problem_id, title, description)) {
            send_error(response, 500, "INTERNAL_ERROR",
                       "failed to update problem");
            return;
        }

        send_json(response, 200, {{"status", "ok"}});
    });

    server.Delete(R"(/api/admin/problems/(\d+))",
                  [&](const httplib::Request& request,
                      httplib::Response& response) {
        if (!require_admin_request(request, response, database, sessions)) {
            return;
        }

        std::uint64_t problem_id = std::stoull(request.matches[1].str());
        Problem existing;
        if (!database.find_problem_by_id(problem_id, existing)) {
            send_error(response, 404, "PROBLEM_NOT_FOUND",
                       "problem not found");
            return;
        }

        if (!database.delete_problem(problem_id)) {
            send_error(response, 500, "INTERNAL_ERROR",
                       "failed to delete problem");
            return;
        }

        send_json(response, 200, {{"status", "ok"}});
    });

    server.Post(R"(/api/admin/problems/(\d+)/testcases)",
                [&](const httplib::Request& request,
                    httplib::Response& response) {
        if (!require_admin_request(request, response, database, sessions)) {
            return;
        }

        std::uint64_t problem_id = std::stoull(request.matches[1].str());
        Problem existing;
        if (!database.find_problem_by_id(problem_id, existing)) {
            send_error(response, 404, "PROBLEM_NOT_FOUND",
                       "problem not found");
            return;
        }

        nlohmann::json body;
        if (!parse_json_body(request, body)) {
            send_error(response, 400, "INVALID_JSON", "invalid JSON body");
            return;
        }

        if (!body.contains("test_cases") ||
            !body["test_cases"].is_array()) {
            send_error(response, 400, "VALIDATION_ERROR",
                       "test_cases array is required");
            return;
        }

        const nlohmann::json& items = body["test_cases"];
        if (items.empty() || items.size() > 5) {
            send_error(response, 400, "VALIDATION_ERROR",
                       "test case count must be between 1 and 5");
            return;
        }

        std::vector<TestCase> test_cases;
        bool has_sample = false;
        int order_index = 1;

        for (const nlohmann::json& item : items) {
            if (!item.is_object() ||
                !item.contains("input") ||
                !item.contains("expected_output")) {
                send_error(
                    response, 400, "VALIDATION_ERROR",
                    "each test case requires input and expected_output");
                return;
            }

            TestCase test_case;
            test_case.problem_id = problem_id;
            test_case.input_data = item["input"].get<std::string>();
            test_case.expected_output_data =
                item["expected_output"].get<std::string>();
            test_case.is_sample = item.value("is_sample", false);
            test_case.order_index = order_index++;
            has_sample = has_sample || test_case.is_sample;
            test_cases.push_back(std::move(test_case));
        }

        if (!has_sample) {
            send_error(response, 400, "VALIDATION_ERROR",
                       "at least one sample test case is required");
            return;
        }

        std::string error;
        if (!database.replace_test_cases(problem_id, test_cases, error)) {
            send_error(response, 500, "INTERNAL_ERROR",
                       "failed to import test cases", {{"detail", error}});
            return;
        }

        send_json(response, 200,
                  {{"status", "ok"},
                   {"count", static_cast<int>(test_cases.size())}});
    });
}
