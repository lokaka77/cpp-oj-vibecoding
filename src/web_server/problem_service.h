#pragma once

#include <httplib.h>

#include "database.h"
#include "oj/models.h"
#include "session.h"

#include <nlohmann/json.hpp>

#include <vector>

void register_problem_routes(httplib::Server& server,
                             Database& database,
                             SessionManager& sessions,
                             const JudgeConfig& judge_config);

inline nlohmann::json problem_list_json(
    const std::vector<ProblemSummary>& problems) {
    nlohmann::json items = nlohmann::json::array();
    for (const ProblemSummary& problem : problems) {
        items.push_back({{"id", problem.id}, {"title", problem.title}});
    }
    return {{"problems", items}};
}

inline nlohmann::json problem_detail_json(
    const Problem& problem,
    const std::vector<TestCase>& samples) {
    nlohmann::json sample_items = nlohmann::json::array();
    for (const TestCase& sample : samples) {
        sample_items.push_back({
            {"id", sample.id},
            {"input", sample.input_data},
            {"expected_output", sample.expected_output_data},
            {"order_index", sample.order_index},
        });
    }

    return {
        {"id", problem.id},
        {"title", problem.title},
        {"description", problem.description},
        {"samples", sample_items},
    };
}
