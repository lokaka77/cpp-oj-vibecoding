#pragma once

#include "config.h"

#include <filesystem>
#include <string>

struct CompileResult {
    bool success = false;
    std::string friendly_message;
    std::string diagnostics;
};

class Compiler {
public:
    explicit Compiler(const JudgeConfig& config);

    CompileResult compile(const std::filesystem::path& source,
                          const std::filesystem::path& executable) const;

private:
    JudgeConfig config_;
};
