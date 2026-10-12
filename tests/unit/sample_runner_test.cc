#include <gtest/gtest.h>

#include "sample_runner.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {

#ifndef OJ_TEST_COMPILER
#define OJ_TEST_COMPILER "g++"
#endif

SampleRunner make_runner(int time_limit_ms = 1000,
                         int memory_limit_mb = 128) {
    JudgeConfig config;
    config.compiler = OJ_TEST_COMPILER;
    std::replace(config.compiler.begin(), config.compiler.end(), '/', '\\');
    config.temp_dir = "./judge_test_tmp";
    config.time_limit_ms = time_limit_ms;
    config.memory_limit_mb = memory_limit_mb;
    return SampleRunner(config);
}

TestCase make_sample(const std::string& input,
                     const std::string& expected_output) {
    TestCase sample;
    sample.input_data = input;
    sample.expected_output_data = expected_output;
    sample.is_sample = true;
    sample.order_index = 1;
    return sample;
}

}  // namespace

TEST(SampleRunnerTest, RejectsEmptyCode) {
    SampleRunResult result = make_runner().run("", {});

    EXPECT_FALSE(result.passed);
    EXPECT_EQ(result.message, "空代码");
}

TEST(SampleRunnerTest, PassesCorrectSample) {
    std::string code =
        "#include <iostream>\n"
        "int main(){int a,b; std::cin>>a>>b; std::cout<<a+b;}\n";
    std::vector<TestCase> samples = {make_sample("1 2\n", "3\n")};

    SampleRunResult result = make_runner().run(code, samples);

    EXPECT_TRUE(result.passed);
    EXPECT_EQ(result.verdict, "AC");
    EXPECT_EQ(result.message, "样例通过");
}

TEST(SampleRunnerTest, DetectsWrongOutput) {
    std::string code =
        "#include <iostream>\n"
        "int main(){std::cout<<0;}\n";
    std::vector<TestCase> samples = {make_sample("1 2\n", "3\n")};

    SampleRunResult result = make_runner().run(code, samples);

    EXPECT_FALSE(result.passed);
    EXPECT_EQ(result.verdict, "WA");
    EXPECT_EQ(result.message, "样例未通过");
}

TEST(SampleRunnerTest, DetectsCompileError) {
    std::string code = "int main( {";

    SampleRunResult result = make_runner().run(code, {});

    EXPECT_FALSE(result.passed);
    EXPECT_EQ(result.verdict, "CE");
    EXPECT_EQ(result.message, "编译错误");
    EXPECT_EQ(result.message.find("error:"), std::string::npos);
}

TEST(SampleRunnerTest, DetectsRuntimeError) {
    std::string code = "int main(){return 7;}\n";
    std::vector<TestCase> samples = {make_sample("", "")};

    SampleRunResult result = make_runner().run(code, samples);

    EXPECT_FALSE(result.passed);
    EXPECT_EQ(result.verdict, "RE");
    EXPECT_EQ(result.message, "运行时错误");
}

TEST(SampleRunnerTest, DetectsTimeLimitExceeded) {
    std::string code =
        "int main(){while(true){}}\n";
    std::vector<TestCase> samples = {make_sample("", "")};

    SampleRunResult result = make_runner(200).run(code, samples);

    EXPECT_FALSE(result.passed);
    EXPECT_EQ(result.verdict, "TLE");
    EXPECT_EQ(result.message, "运行超时");
}

TEST(SampleRunnerTest, DetectsMemoryLimitExceeded) {
    std::string code =
        "#include <vector>\n"
        "int main(){std::vector<char> data(256 * 1024 * 1024, 1);"
        " return data[0];}\n";
    std::vector<TestCase> samples = {make_sample("", "")};

    SampleRunResult result = make_runner(1000, 32).run(code, samples);

    EXPECT_FALSE(result.passed);
    EXPECT_EQ(result.verdict, "RE");
    EXPECT_EQ(result.message, "运行时错误");
}
