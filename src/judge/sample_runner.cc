#include "sample_runner.h"

#include "compiler.h"
#include "logging.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

std::string trim_trailing_whitespace(const std::string& value) {
    std::size_t end = value.size();
    while (end > 0) {
        char current = value[end - 1];
        if (current != ' ' && current != '\t' &&
            current != '\r' && current != '\n') {
            break;
        }
        --end;
    }
    return value.substr(0, end);
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void write_file(const std::filesystem::path& path,
                const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
}

class TempDirectory {
public:
    explicit TempDirectory(const std::filesystem::path& base) {
        auto timestamp =
            std::chrono::high_resolution_clock::now().time_since_epoch().count();
        path_ = base /
                ("oj_sample_" + std::to_string(timestamp));
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

}  // namespace

SampleRunner::SampleRunner(const JudgeConfig& config)
    : config_(config) {}

SampleRunResult SampleRunner::run(const std::string& code,
                                  const std::vector<TestCase>& samples) const {
    if (code.empty()) {
        return {false, "CE", "空代码"};
    }

    try {
        TempDirectory temp_dir(std::filesystem::absolute(config_.temp_dir));
        std::filesystem::path source = temp_dir.path() / "main.cpp";
        std::filesystem::path executable = temp_dir.path() / "program.exe";

        write_file(source, code);

        Compiler compiler(config_);
        CompileResult compile_result = compiler.compile(source, executable);
        if (!compile_result.success) {
            log_warning("Compilation failed: " + compile_result.diagnostics);
            return {false, "CE", "编译错误"};
        }

        for (std::size_t index = 0; index < samples.size(); ++index) {
            std::filesystem::path input_file =
                temp_dir.path() / ("input_" + std::to_string(index) + ".txt");
            std::filesystem::path output_file =
                temp_dir.path() / ("output_" + std::to_string(index) + ".txt");

            write_file(input_file, samples[index].input_data);

            std::string run_command =
                executable.string() + " < " + input_file.string() +
                " > " + output_file.string();

            if (std::system(run_command.c_str()) != 0) {
                log_error("Sample run command failed: " + run_command);
                return {false, "RE", "运行时错误"};
            }

            std::string actual =
                trim_trailing_whitespace(read_file(output_file));
            std::string expected = trim_trailing_whitespace(
                samples[index].expected_output_data);

            if (actual != expected) {
                return {false, "WA", "样例未通过"};
            }
        }

        return {true, "AC", "样例通过"};
    } catch (const std::exception&) {
        return {false, "RE", "运行失败"};
    }
}
