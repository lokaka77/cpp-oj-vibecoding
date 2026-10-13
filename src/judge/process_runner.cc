#include "process_runner.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <mutex>
#include <thread>
#include <vector>

namespace {

void disable_error_reporting() {
    static std::once_flag once;
    std::call_once(once, []() {
        // 让崩溃的子进程直接退出：否则 Windows 错误报告会挂住进程，
        // 判题侧等满时间限制后会把它误判成 TLE。
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    });
}

SECURITY_ATTRIBUTES inheritable_attributes() {
    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(attributes);
    attributes.bInheritHandle = TRUE;
    return attributes;
}

HANDLE open_input(const std::filesystem::path& path) {
    SECURITY_ATTRIBUTES attributes = inheritable_attributes();
    if (path.empty()) {
        return CreateFileW(L"NUL",
                           GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           &attributes,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           nullptr);
    }

    return CreateFileW(path.wstring().c_str(),
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       &attributes,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       nullptr);
}

HANDLE open_output(const std::filesystem::path& path) {
    SECURITY_ATTRIBUTES attributes = inheritable_attributes();
    return CreateFileW(path.wstring().c_str(),
                       GENERIC_WRITE,
                       FILE_SHARE_READ,
                       &attributes,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       nullptr);
}

// 并发判题时，刚创建的输入/输出文件可能被杀毒软件短暂占用，
// 这里做几次退避重试，避免把基础设施抖动记成用户的运行时错误。
HANDLE open_with_retry(HANDLE (*open_file)(const std::filesystem::path&),
                       const std::filesystem::path& path) {
    for (int attempt = 0; attempt < 4; ++attempt) {
        HANDLE handle = open_file(path);
        if (handle != INVALID_HANDLE_VALUE) {
            return handle;
        }
        std::this_thread::sleep_for(
            std::chrono::milliseconds(50 * (attempt + 1)));
    }
    return INVALID_HANDLE_VALUE;
}

// 构造受限令牌：把当前用户的 SID 设为 deny-only，
// 这样用户代码读不到项目文件（config.yaml、数据库文件等），
// 但仍能读 System32 下的系统 DLL（它们的权限给的是 Everyone/Users）。
HANDLE restricted_token() {
    static std::once_flag once;
    static HANDLE token = nullptr;

    std::call_once(once, []() {
        HANDLE current = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(),
                              TOKEN_DUPLICATE | TOKEN_QUERY,
                              &current)) {
            return;
        }

        DWORD size = 0;
        GetTokenInformation(current, TokenUser, nullptr, 0, &size);
        std::vector<char> buffer(size);
        if (size == 0 ||
            !GetTokenInformation(current, TokenUser, buffer.data(), size,
                                 &size)) {
            CloseHandle(current);
            return;
        }

        auto* user = reinterpret_cast<TOKEN_USER*>(buffer.data());
        SID_AND_ATTRIBUTES deny{};
        deny.Sid = user->User.Sid;
        deny.Attributes = 0;

        HANDLE created = nullptr;
        if (CreateRestrictedToken(current, DISABLE_MAX_PRIVILEGE,
                                  1, &deny,
                                  0, nullptr,
                                  0, nullptr,
                                  &created)) {
            token = created;
        }
        CloseHandle(current);
    });

    return token;
}

}  // namespace

bool ensure_restricted_access(const std::filesystem::path& directory) {
    std::error_code error;
    if (!std::filesystem::exists(directory, error)) {
        return false;
    }

    wchar_t system_root[MAX_PATH]{};
    if (GetEnvironmentVariableW(L"SystemRoot", system_root, MAX_PATH) == 0) {
        return false;
    }

    // 通过 icacls 给目录追加 Everyone 的读写权限（固定命令，不含任何用户输入）。
    const std::wstring icacls =
        std::wstring(system_root) + L"\\System32\\icacls.exe";
    std::wstring command = quote_windows_argument(icacls) + L" " +
                           quote_windows_argument(directory.wstring()) +
                           L" /grant \"*S-1-1-0:(OI)(CI)M\"";

    std::vector<wchar_t> buffer(command.begin(), command.end());
    buffer.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, buffer.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup,
                        &process)) {
        return false;
    }

    WaitForSingleObject(process.hProcess, 10000);
    DWORD exit_code = 1;
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return exit_code == 0;
}

FixedRunPath::FixedRunPath(const std::filesystem::path& base_directory) {
    path_ = base_directory / "runner" / "program.exe";
    std::error_code error;
    std::filesystem::create_directories(path_.parent_path(), error);

    HANDLE mutex = CreateMutexW(nullptr, FALSE, L"Local\\CodeHunterJudgeRun");
    if (mutex != nullptr) {
        WaitForSingleObject(mutex, INFINITE);
    }
    mutex_ = mutex;
}

FixedRunPath::~FixedRunPath() {
    std::error_code error;
    std::filesystem::remove(path_, error);

    if (mutex_ != nullptr) {
        auto* mutex = static_cast<HANDLE>(mutex_);
        ReleaseMutex(mutex);
        CloseHandle(mutex);
    }
}

bool FixedRunPath::stage(const std::filesystem::path& executable) {
    std::error_code error;
    std::filesystem::copy_file(executable, path_,
                               std::filesystem::copy_options::overwrite_existing,
                               error);
    return !error;
}

const std::filesystem::path& FixedRunPath::path() const {
    return path_;
}

std::wstring quote_windows_argument(const std::wstring& argument) {
    if (argument.empty()) {
        return L"\"\"";
    }
    if (argument.find_first_of(L" \t\"") == std::wstring::npos) {
        return argument;
    }

    std::wstring quoted = L"\"";
    int backslashes = 0;
    for (wchar_t character : argument) {
        if (character == L'\\') {
            ++backslashes;
        } else if (character == L'"') {
            quoted.append(static_cast<std::size_t>(backslashes * 2 + 1),
                          L'\\');
            quoted.push_back(L'"');
            backslashes = 0;
        } else {
            quoted.append(static_cast<std::size_t>(backslashes), L'\\');
            backslashes = 0;
            quoted.push_back(character);
        }
    }
    quoted.append(static_cast<std::size_t>(backslashes * 2), L'\\');
    quoted.push_back(L'"');
    return quoted;
}

ProcessResult ProcessRunner::run(
    const std::filesystem::path& executable,
    const std::wstring& arguments,
    const std::filesystem::path& input_file,
    const std::filesystem::path& output_file,
    const std::filesystem::path& error_file,
    std::size_t memory_limit_mb,
    std::chrono::milliseconds timeout,
    bool restrict_file_access,
    bool limit_child_processes) {
    ProcessResult result;

    disable_error_reporting();

    HANDLE input_handle = open_with_retry(open_input, input_file);
    HANDLE output_handle = open_with_retry(open_output, output_file);
    HANDLE error_handle = error_file == output_file
                              ? output_handle
                              : open_with_retry(open_output, error_file);

    if (input_handle == INVALID_HANDLE_VALUE ||
        output_handle == INVALID_HANDLE_VALUE ||
        error_handle == INVALID_HANDLE_VALUE) {
        result.error = "Failed to open process redirection files (error " +
                       std::to_string(GetLastError()) + ")";
        if (input_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(input_handle);
        }
        if (output_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(output_handle);
        }
        if (error_handle != INVALID_HANDLE_VALUE &&
            error_handle != output_handle) {
            CloseHandle(error_handle);
        }
        return result;
    }

    std::wstring command_line =
        quote_windows_argument(executable.wstring());
    if (!arguments.empty()) {
        command_line += L" " + arguments;
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input_handle;
    startup.hStdOutput = output_handle;
    startup.hStdError = error_handle;

    // 子进程工作目录固定为可执行文件所在目录（判题临时目录），
    // 避免用户代码用相对路径读到项目目录下的配置文件等敏感文件。
    // 注意：编译器是相对路径（g++，靠 PATH 解析），此时 parent_path()
    // 为空，必须回退为继承父进程工作目录，否则 CreateProcess 会失败。
    const std::wstring working_directory =
        executable.parent_path().wstring();

    PROCESS_INFORMATION process{};
    BOOL created = FALSE;
    DWORD create_error = 0;
    for (int attempt = 0; attempt < 3 && !created; ++attempt) {
        if (attempt > 0) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(50 * attempt));
        }

        std::vector<wchar_t> command_buffer(
            command_line.begin(), command_line.end());
        command_buffer.push_back(L'\0');

        // 受限令牌方案在标准用户权限下会被窗口站 ACL 拒绝
        // （CreateProcessAsUser 返回 ERROR_ACCESS_DENIED），
        // 因此默认不启用；保留开关供将来配合 AppContainer 使用。
        HANDLE token = restrict_file_access ? restricted_token() : nullptr;
        if (token != nullptr) {
            created = CreateProcessAsUserW(
                token,
                nullptr,
                command_buffer.data(),
                nullptr,
                nullptr,
                TRUE,
                CREATE_SUSPENDED | CREATE_NO_WINDOW,
                nullptr,
                working_directory.empty() ? nullptr
                                          : working_directory.c_str(),
                &startup,
                &process);
        } else {
            created = CreateProcessW(
                nullptr,
                command_buffer.data(),
                nullptr,
                nullptr,
                TRUE,
                CREATE_SUSPENDED | CREATE_NO_WINDOW,
                nullptr,
                working_directory.empty() ? nullptr
                                          : working_directory.c_str(),
                &startup,
                &process);
        }
        if (!created) {
            create_error = GetLastError();
        }
    }

    if (!created) {
        result.error = "CreateProcess failed with error " +
                       std::to_string(create_error);
        CloseHandle(input_handle);
        CloseHandle(output_handle);
        if (error_handle != output_handle) {
            CloseHandle(error_handle);
        }
        return result;
    }

    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (job != nullptr) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags =
            JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (memory_limit_mb > 0) {
            const SIZE_T bytes = static_cast<SIZE_T>(memory_limit_mb) *
                                 1024ULL * 1024ULL;
            limits.BasicLimitInformation.LimitFlags |=
                JOB_OBJECT_LIMIT_PROCESS_MEMORY |
                JOB_OBJECT_LIMIT_JOB_MEMORY;
            limits.ProcessMemoryLimit = bytes;
            limits.JobMemoryLimit = bytes;
        }
        if (limit_child_processes) {
            // 判题程序不允许创建子进程：否则可以用 system() 拉起
            // curl / powershell 等程序绕过按路径配置的防火墙规则。
            limits.BasicLimitInformation.LimitFlags |=
                JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
            limits.BasicLimitInformation.ActiveProcessLimit = 1;
        }
        SetInformationJobObject(job,
                                JobObjectExtendedLimitInformation,
                                &limits,
                                sizeof(limits));
        if (!AssignProcessToJobObject(job, process.hProcess)) {
            CloseHandle(job);
            job = nullptr;
        }
    }

    ResumeThread(process.hThread);

    const DWORD wait_timeout =
        static_cast<DWORD>(timeout.count() > 0 ? timeout.count() : 1);
    const DWORD wait_result =
        WaitForSingleObject(process.hProcess, wait_timeout);

    if (wait_result == WAIT_TIMEOUT) {
        result.timed_out = true;
        if (job != nullptr) {
            TerminateJobObject(job, 1);
        } else {
            TerminateProcess(process.hProcess, 1);
        }
        WaitForSingleObject(process.hProcess, 5000);
    }

    DWORD exit_code = 0;
    GetExitCodeProcess(process.hProcess, &exit_code);
    result.started = true;
    result.exit_code = exit_code;
    result.error = wait_result == WAIT_FAILED
                       ? "WaitForSingleObject failed"
                       : "";

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (job != nullptr) {
        CloseHandle(job);
    }
    CloseHandle(input_handle);
    CloseHandle(output_handle);
    if (error_handle != output_handle) {
        CloseHandle(error_handle);
    }

    return result;
}
