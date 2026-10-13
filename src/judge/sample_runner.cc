#include "sample_runner.h"

#include "compiler.h"
#include "logging.h"
#include "output_comparer.h"
#include "process_runner.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

namespace {

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
        const std::filesystem::path temp_root =
            std::filesystem::absolute(config_.temp_dir);
        std::error_code directory_error;
        std::filesystem::create_directories(temp_root, directory_error);

        // 受限令牌下的判题进程只能访问 Everyone 有权限的路径，
        // 因此先给判题临时根目录追加 Everyone 权限（每个进程只做一次）。
        static std::once_flag access_once;
        std::call_once(access_once, [&temp_root]() {
            if (!ensure_restricted_access(temp_root)) {
                log_warning("Failed to grant Everyone access on " +
                            temp_root.string());
            }
        });

        TempDirectory temp_dir(temp_root);
        std::filesystem::path source = temp_dir.path() / "main.cpp";
        std::filesystem::path executable = temp_dir.path() / "program.exe";

        write_file(source, code);

        Compiler compiler(config_);
        CompileResult compile_result = compiler.compile(source, executable);
        if (!compile_result.success) {
            log_warning("Compilation failed: " + compile_result.diagnostics);
            const std::string message =
                compile_result.friendly_message.empty()
                    ? "编译错误"
                    : compile_result.friendly_message;
            return {false, "CE", message};
        }

        // 统一落到固定路径执行：便于按路径配置防火墙出网阻断规则。
        FixedRunPath run_path(temp_root);
        if (!run_path.stage(executable)) {
            log_error("Failed to stage runner executable");
            return {false, "RE", "运行失败"};
        }

        for (std::size_t index = 0; index < samples.size(); ++index) {
            std::filesystem::path input_file =
                temp_dir.path() / ("input_" + std::to_string(index) + ".txt");
            std::filesystem::path output_file =
                temp_dir.path() / ("output_" + std::to_string(index) + ".txt");
            std::filesystem::path error_file =
                temp_dir.path() / ("error_" + std::to_string(index) + ".txt");

            write_file(input_file, samples[index].input_data);

            ProcessResult process = ProcessRunner::run(
                run_path.path(),
                L"",
                input_file,
                output_file,
                error_file,
                static_cast<std::size_t>(config_.memory_limit_mb),
                std::chrono::milliseconds(config_.time_limit_ms),
                false,
                true);

            if (!process.started) {
                log_error("Failed to start sample process: " + process.error);
                return {false, "RE", "运行时错误"};
            }
            if (process.timed_out) {
                return {false, "TLE", "运行超时"};
            }
            if (process.exit_code != 0) {
                return {false, "RE", "运行时错误"};
            }

            const std::string actual = read_file(output_file);
            if (!OutputComparer::matches(
                    actual, samples[index].expected_output_data)) {
                return {false, "WA", "样例未通过"};
            }
        }

        return {true, "AC", "样例通过"};
    } catch (const std::exception&) {
        return {false, "RE", "运行失败"};
    }
}
