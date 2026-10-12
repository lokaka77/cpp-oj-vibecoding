#include "problem_service.h"

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

}  // namespace

void register_problem_routes(httplib::Server& server, Database& database) {
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
}
