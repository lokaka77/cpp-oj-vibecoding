#include "compiler.h"

#include <cstdlib>
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

    std::string command =
        config_.compiler + " -std=c++17 " + source.string() +
        " -o " + executable.string() + " > " + log_path.string() + " 2>&1";

    CompileResult result;
    result.success = std::system(command.c_str()) == 0;
    result.diagnostics = read_file(log_path);
    return result;
}
