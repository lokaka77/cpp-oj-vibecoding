#include "compiler.h"

#include "process_runner.h"

#include <chrono>
#include <fstream>
#include <sstream>

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

}  // namespace

Compiler::Compiler(const JudgeConfig& config)
    : config_(config) {}

CompileResult Compiler::compile(
    const std::filesystem::path& source,
    const std::filesystem::path& executable) const {
    const std::filesystem::path log_path =
        executable.parent_path() / "compile.log";

    const std::wstring arguments =
        L"-std=c++17 " + quote_windows_argument(source.wstring()) +
        L" -o " + quote_windows_argument(executable.wstring());

    ProcessResult process = ProcessRunner::run(
        std::filesystem::path(config_.compiler),
        arguments,
        {},
        log_path,
        log_path,
        std::chrono::seconds(10));

    CompileResult result;
    result.success = process.started && !process.timed_out &&
                     process.exit_code == 0;
    result.diagnostics = read_file(log_path);
    return result;
}
