#pragma once

#include <cstdint>
#include <string>

struct ProblemSummary {
    std::uint64_t id = 0;
    std::string title;
};

struct Problem {
    std::uint64_t id = 0;
    std::string title;
    std::string description;
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
