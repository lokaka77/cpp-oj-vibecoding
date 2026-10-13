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
        std::chrono::milliseconds timeout,
        bool restrict_file_access = false,
        bool limit_child_processes = false);
};

std::wstring quote_windows_argument(const std::wstring& argument);

// 给目录追加 Everyone 的读写权限，使受限令牌下的判题子进程仍能读写自己的临时文件。
bool ensure_restricted_access(const std::filesystem::path& directory);

// 固定执行路径：判题产物统一落到 <base>/runner/program.exe，
// 便于用防火墙规则按路径阻断出网；同一时刻只允许一个判题进程占用该路径。
class FixedRunPath {
public:
    explicit FixedRunPath(const std::filesystem::path& base_directory);
    ~FixedRunPath();

    FixedRunPath(const FixedRunPath&) = delete;
    FixedRunPath& operator=(const FixedRunPath&) = delete;

    bool stage(const std::filesystem::path& executable);
    const std::filesystem::path& path() const;

private:
    std::filesystem::path path_;
    void* mutex_ = nullptr;
};
