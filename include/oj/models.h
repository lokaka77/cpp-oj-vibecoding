#pragma once

#include <cstdint>
#include <string>

struct ProblemSummary {
    std::uint64_t id = 0;
    std::string title;
    std::string difficulty;
};

struct Problem {
    std::uint64_t id = 0;
    std::string title;
    std::string description;
    std::string difficulty;
};

struct TestCase {
    std::uint64_t id = 0;
    std::uint64_t problem_id = 0;
    std::string input_data;
    std::string expected_output_data;
    bool is_sample = false;
    int order_index = 0;
};

struct JudgeTask {
    std::uint64_t submission_id = 0;
    std::uint64_t user_id = 0;
    std::uint64_t problem_id = 0;
    std::string code;
};

struct UserSummary {
    std::uint64_t id = 0;
    std::string username;
    std::string role;
    std::string created_at;
};
