#include "problem_service.h"

#include "sample_runner.h"

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

}  // namespace

void register_problem_routes(httplib::Server& server,
                             Database& database,
                             SessionManager& sessions,
                             const JudgeConfig& judge_config) {
    server.Get("/api/problems", [&](const httplib::Request&,
                                    httplib::Response& response) {
        std::vector<ProblemSummary> problems;
        if (!database.list_problems(problems)) {
            send_json(response, 500, {{"error", "failed to load problems"}});
            return;
        }

        send_json(response, 200, problem_list_json(problems));
    });

    server.Get(R"(/api/problems/(\d+))",
               [&](const httplib::Request& request,
                   httplib::Response& response) {
        std::uint64_t problem_id = std::stoull(request.matches[1].str());

        Problem problem;
        if (!database.find_problem_by_id(problem_id, problem)) {
            send_json(response, 404, {{"error", "problem not found"}});
            return;
        }

        std::vector<TestCase> test_cases;
        if (!database.list_sample_test_cases(problem_id, test_cases)) {
            send_json(response, 500, {{"error", "failed to load test cases"}});
            return;
        }

        send_json(response, 200, problem_detail_json(problem, test_cases));
    });

    server.Post(R"(/api/problems/(\d+)/run)",
                [&](const httplib::Request& request,
                    httplib::Response& response) {
        std::uint64_t user_id = 0;
        if (!sessions.get(cookie_value(request, "session_id"), user_id)) {
            send_json(response, 401, {{"error", "not authenticated"}});
            return;
        }

        nlohmann::json body;
        try {
            body = nlohmann::json::parse(request.body);
        } catch (const std::exception&) {
            send_json(response, 400, {{"error", "invalid JSON body"}});
            return;
        }

        std::string code = body.value("code", "");
        if (code.empty()) {
            send_json(response, 400, {{"error", "code is required"}});
            return;
        }

        std::uint64_t problem_id = std::stoull(request.matches[1].str());
        Problem problem;
        if (!database.find_problem_by_id(problem_id, problem)) {
            send_json(response, 404, {{"error", "problem not found"}});
            return;
        }

        std::vector<TestCase> samples;
        if (!database.list_sample_test_cases(problem_id, samples)) {
            send_json(response, 500, {{"error", "failed to load samples"}});
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
        std::uint64_t user_id = 0;
        if (!sessions.get(cookie_value(request, "session_id"), user_id)) {
            send_json(response, 401, {{"error", "not authenticated"}});
            return;
        }

        nlohmann::json body;
        try {
            body = nlohmann::json::parse(request.body);
        } catch (const std::exception&) {
            send_json(response, 400, {{"error", "invalid JSON body"}});
            return;
        }

        std::string code = body.value("code", "");
        if (code.empty()) {
            send_json(response, 400, {{"error", "code is required"}});
            return;
        }

        std::uint64_t problem_id = std::stoull(request.matches[1].str());
        Problem problem;
        if (!database.find_problem_by_id(problem_id, problem)) {
            send_json(response, 404, {{"error", "problem not found"}});
            return;
        }

        std::vector<TestCase> test_cases;
        if (!database.list_all_test_cases(problem_id, test_cases)) {
            send_json(response, 500, {{"error", "failed to load test cases"}});
            return;
        }

        SampleRunner runner(judge_config);
        SampleRunResult result = runner.run(code, test_cases);

        std::uint64_t submission_id = 0;
        if (!database.insert_submission(user_id, problem_id, code,
                                        result.verdict, submission_id)) {
            send_json(response, 500, {{"error", "failed to save submission"}});
            return;
        }

        send_json(response, 200,
                  {{"submission_id", submission_id},
                   {"verdict", result.verdict}});
    });
}
