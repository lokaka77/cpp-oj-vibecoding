#pragma once

#include <ctime>
#include <iostream>
#include <mutex>
#include <string>

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
};

inline LogLevel g_log_level = LogLevel::Info;
inline std::mutex g_log_mutex;

inline LogLevel parse_log_level(const std::string& value) {
    if (value == "debug") {
        return LogLevel::Debug;
    }
    if (value == "warning" || value == "warn") {
        return LogLevel::Warning;
    }
    if (value == "error") {
        return LogLevel::Error;
    }
    return LogLevel::Info;
}

inline const char* log_level_name(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Warning:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    case LogLevel::Info:
    default:
        return "INFO";
    }
}

inline std::string current_time_text() {
    std::time_t now = std::time(nullptr);
    std::tm local_time{};
#if defined(_WIN32)
    localtime_s(&local_time, &now);
#else
    localtime_r(&now, &local_time);
#endif

    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_time);
    return buffer;
}

inline void log_message(LogLevel level, const std::string& message) {
    if (level < g_log_level) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::ostream& output = level >= LogLevel::Warning ? std::cerr : std::cout;
    output << '[' << current_time_text() << "][" << log_level_name(level)
           << "] " << message << '\n'
           << std::flush;
}

inline void log_debug(const std::string& message) {
    log_message(LogLevel::Debug, message);
}

inline void log_info(const std::string& message) {
    log_message(LogLevel::Info, message);
}

inline void log_warning(const std::string& message) {
    log_message(LogLevel::Warning, message);
}

inline void log_error(const std::string& message) {
    log_message(LogLevel::Error, message);
}
