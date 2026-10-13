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

}  // namespace

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
    std::chrono::milliseconds timeout) {
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

        created = CreateProcessW(
            nullptr,
            command_buffer.data(),
            nullptr,
            nullptr,
            TRUE,
            CREATE_SUSPENDED | CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startup,
            &process);
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
