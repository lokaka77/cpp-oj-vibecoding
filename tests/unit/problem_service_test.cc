#include <gtest/gtest.h>

#include "problem_service.h"

#include <nlohmann/json.hpp>

#include <vector>

TEST(ProblemServiceTest, BuildsEmptyProblemList) {
    nlohmann::json result = problem_list_json({});

    ASSERT_TRUE(result.contains("problems"));
    EXPECT_TRUE(result["problems"].is_array());
    EXPECT_TRUE(result["problems"].empty());
}

TEST(ProblemServiceTest, BuildsProblemListItems) {
    std::vector<ProblemSummary> problems;
    problems.push_back({1, "A+B Problem"});
    problems.push_back({2, "Sort Array"});

    nlohmann::json result = problem_list_json(problems);

    ASSERT_EQ(result["problems"].size(), 2);
    EXPECT_EQ(result["problems"][0]["id"], 1);
    EXPECT_EQ(result["problems"][0]["title"], "A+B Problem");
    EXPECT_EQ(result["problems"][1]["id"], 2);
    EXPECT_EQ(result["problems"][1]["title"], "Sort Array");
}

TEST(ProblemServiceTest, BuildsProblemDetailWithoutSamples) {
    Problem problem{7, "Hello", "Print hello"};

    nlohmann::json result = problem_detail_json(problem, {});

    EXPECT_EQ(result["id"], 7);
    EXPECT_EQ(result["title"], "Hello");
    EXPECT_EQ(result["description"], "Print hello");
    EXPECT_TRUE(result["samples"].is_array());
    EXPECT_TRUE(result["samples"].empty());
}

TEST(ProblemServiceTest, BuildsVisibleSampleTestCases) {
    Problem problem{7, "Hello", "Print hello"};
    std::vector<TestCase> samples;
    samples.push_back({1, 7, "1 2\n", "3\n", true, 1});
    samples.push_back({2, 7, "10 20\n", "30\n", true, 2});

    nlohmann::json result = problem_detail_json(problem, samples);

    ASSERT_EQ(result["samples"].size(), 2);
    EXPECT_EQ(result["samples"][0]["id"], 1);
    EXPECT_EQ(result["samples"][0]["input"], "1 2\n");
    EXPECT_EQ(result["samples"][0]["expected_output"], "3\n");
    EXPECT_EQ(result["samples"][0]["order_index"], 1);
    EXPECT_FALSE(result["samples"][0].contains("problem_id"));
}
