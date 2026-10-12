#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <string>

struct ProcessResult {
    bool started = false;
    bool timed_out = false;
    unsigned long exit_code = 0;
    std::string error;
};

class ProcessRunner {
public:
    static ProcessResult run(
        const std::filesystem::path& executable,
        const std::wstring& arguments,
        const std::filesystem::path& input_file,
        const std::filesystem::path& output_file,
        const std::filesystem::path& error_file,
        std::size_t memory_limit_mb,
        std::chrono::milliseconds timeout);
};

std::wstring quote_windows_argument(const std::wstring& argument);
