#include <gtest/gtest.h>

#include "compiler.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

#ifndef OJ_TEST_COMPILER
#define OJ_TEST_COMPILER "g++"
#endif

class TempDirectory {
public:
    TempDirectory() {
        auto timestamp =
            std::chrono::high_resolution_clock::now().time_since_epoch().count();
        path_ = std::filesystem::absolute("./compiler_test_tmp") /
                std::to_string(timestamp);
        std::filesystem::create_directories(path_);
    }

    ~TempDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    const std::filesystem::path& path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

void write_file(const std::filesystem::path& path,
                const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    output << content;
}

JudgeConfig test_config() {
    JudgeConfig config;
    config.compiler = OJ_TEST_COMPILER;
    std::replace(config.compiler.begin(), config.compiler.end(), '/', '\\');
    return config;
}

}  // namespace

TEST(CompilerTest, CompilesCpp17StructuredBindings) {
    TempDirectory temp;
    const std::filesystem::path source = temp.path() / "main.cpp";
    const std::filesystem::path executable = temp.path() / "program.exe";

    write_file(source,
               "#include <tuple>\n"
               "int main() {\n"
               "  auto values = std::make_tuple(1, 2);\n"
               "  auto [a, b] = values;\n"
               "  return a + b == 3 ? 0 : 1;\n"
               "}\n");

    Compiler compiler(test_config());
    CompileResult result = compiler.compile(source, executable);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(std::filesystem::exists(executable));
}

TEST(CompilerTest, ReportsDiagnosticsForInvalidCode) {
    TempDirectory temp;
    const std::filesystem::path source = temp.path() / "main.cpp";
    const std::filesystem::path executable = temp.path() / "program.exe";

    write_file(source, "int main( {\n");

    Compiler compiler(test_config());
    CompileResult result = compiler.compile(source, executable);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.diagnostics.empty());
}
