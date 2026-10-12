#include "problem_service.h"

#include "auth_middleware.h"
#include "http_utils.h"
#include "sample_runner.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

void register_problem_routes(httplib::Server& server,
                             Database& database,
                             SessionManager& sessions,
                             const JudgeConfig& judge_config,
                             SubmissionRateLimiter& rate_limiter) {
    server.Get("/api/problems", [&](const httplib::Request&,
                                    httplib::Response& response) {
        std::vector<ProblemSummary> problems;
        if (!database.list_problems(problems)) {
            send_error(response, 500, "INTERNAL_ERROR",
                       "failed to load problems");
            return;
        }

        send_json(response, 200, problem_list_json(problems));
    });

    server.Get("/api/problems/solved",
               [&](const httplib::Request& request,
                   httplib::Response& response) {
        AuthUser user;
        if (!require_authenticated(request, response, database, sessions,
                                   user)) {
            return;
        }

        std::vector<std::uint64_t> problem_ids;
        if (!database.list_solved_problem_ids(user.id, problem_ids)) {
            send_error(response, 500, "INTERNAL_ERROR",
                       "failed to load solved problems");
            return;
        }

        nlohmann::json items = nlohmann::json::array();
        for (std::uint64_t problem_id : problem_ids) {
            items.push_back(problem_id);
        }
        send_json(response, 200, {{"problem_ids", items}});
    });

    server.Get(R"(/api/problems/(\d+))",
               [&](const httplib::Request& request,
                   httplib::Response& response) {
        AuthUser user;
        if (!require_authenticated(request, response, database, sessions,
                                   user)) {
            return;
        }

        std::uint64_t problem_id = std::stoull(request.matches[1].str());

        Problem problem;
        if (!database.find_problem_by_id(problem_id, problem)) {
            send_error(response, 404, "PROBLEM_NOT_FOUND",
                       "problem not found");
            return;
        }

        std::vector<TestCase> test_cases;
        if (!database.list_sample_test_cases(problem_id, test_cases)) {
            send_error(response, 500, "INTERNAL_ERROR",
                       "failed to load test cases");
            return;
        }

        send_json(response, 200, problem_detail_json(problem, test_cases));
    });

    server.Post(R"(/api/problems/(\d+)/run)",
                [&](const httplib::Request& request,
                    httplib::Response& response) {
        AuthUser user;
        if (!require_authenticated(request, response, database, sessions,
                                   user)) {
            return;
        }

        nlohmann::json body;
        if (!parse_json_body(request, body)) {
            send_error(response, 400, "INVALID_JSON", "invalid JSON body");
            return;
        }

        std::string code = body.value("code", "");
        if (code.empty()) {
            send_error(response, 400, "VALIDATION_ERROR",
                       "code is required");
            return;
        }

        std::uint64_t problem_id = std::stoull(request.matches[1].str());
        Problem problem;
        if (!database.find_problem_by_id(problem_id, problem)) {
            send_error(response, 404, "PROBLEM_NOT_FOUND",
                       "problem not found");
            return;
        }

        std::vector<TestCase> samples;
        if (!database.list_sample_test_cases(problem_id, samples)) {
            send_error(response, 500, "INTERNAL_ERROR",
                       "failed to load samples");
            return;
        }

        SampleRunner runner(judge_config);
        SampleRunResult result = runner.run(code, samples);
        send_json(response, 200,
                  {{"passed", result.passed}, {"message", result.message}});
    });

    server.Post(R"(/api/problems/(\d+)/submit)",
                [&](const httplib::Request& request,
                    httplib::Response& response) {
        AuthUser user;
        if (!require_authenticated(request, response, database, sessions,
                                   user)) {
            return;
        }

        if (!rate_limiter.allow(user.id)) {
            const auto retry_after = rate_limiter.retry_after(user.id);
            send_error(response, 429, "RATE_LIMITED",
                       "submission too frequent",
                       {{"retry_after_ms", retry_after.count()}});
            return;
        }

        nlohmann::json body;
        if (!parse_json_body(request, body)) {
            send_error(response, 400, "INVALID_JSON", "invalid JSON body");
            return;
        }

        std::string code = body.value("code", "");
        if (code.empty()) {
            send_error(response, 400, "VALIDATION_ERROR",
                       "code is required");
            return;
        }

        std::uint64_t problem_id = std::stoull(request.matches[1].str());
        Problem problem;
        if (!database.find_problem_by_id(problem_id, problem)) {
            send_error(response, 404, "PROBLEM_NOT_FOUND",
                       "problem not found");
            return;
        }

        std::uint64_t submission_id = 0;
        if (!database.insert_pending_submission(user.id, problem_id, code,
                                                submission_id)) {
            send_error(response, 500, "INTERNAL_ERROR",
                       "failed to save submission");
            return;
        }

        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (std::chrono::steady_clock::now() < deadline) {
            std::string status;
            std::string verdict;
            if (!database.get_submission_result(submission_id, status,
                                                verdict)) {
                send_error(response, 500, "INTERNAL_ERROR",
                           "failed to read submission result");
                return;
            }

            if (status == "done") {
                send_json(response, 200,
                          {{"submission_id", submission_id},
                           {"verdict", verdict}});
                return;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        send_error(response, 503, "JUDGE_TIMEOUT",
                   "judge worker did not finish in time");
    });
}
