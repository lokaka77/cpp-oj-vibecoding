#include <gtest/gtest.h>

#include "process_runner.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

std::filesystem::path temp_dir() {
    std::filesystem::path path =
        std::filesystem::absolute("./process_runner_test_tmp");
    std::filesystem::create_directories(path);
    return path;
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

}  // namespace

TEST(ProcessRunnerTest, RunsProcessAndCapturesOutput) {
    const std::filesystem::path dir = temp_dir();
    const std::filesystem::path output = dir / "echo_output.txt";
    const std::filesystem::path error = dir / "echo_error.txt";

    ProcessResult result = ProcessRunner::run(
        "C:\\Windows\\System32\\cmd.exe",
        L"/c echo hello",
        {},
        output,
        error,
        0,
        std::chrono::seconds(2));

    ASSERT_TRUE(result.started);
    EXPECT_FALSE(result.timed_out);
    EXPECT_EQ(result.exit_code, 0U);
    EXPECT_NE(read_file(output).find("hello"), std::string::npos);

    std::error_code cleanup_error;
    std::filesystem::remove_all(dir, cleanup_error);
}

TEST(ProcessRunnerTest, TerminatesTimedOutProcess) {
    const std::filesystem::path dir = temp_dir();
    const std::filesystem::path output = dir / "ping_output.txt";
    const std::filesystem::path error = dir / "ping_error.txt";

    ProcessResult result = ProcessRunner::run(
        "C:\\Windows\\System32\\cmd.exe",
        L"/c ping -n 10 127.0.0.1",
        {},
        output,
        error,
        0,
        std::chrono::milliseconds(300));

    ASSERT_TRUE(result.started);
    EXPECT_TRUE(result.timed_out);

    std::error_code cleanup_error;
    std::filesystem::remove_all(dir, cleanup_error);
}
