#pragma once

#include <string>

struct DatabaseConfig {
    std::string host = "127.0.0.1";
    int port = 3306;
    std::string user = "root";
    std::string password;
    std::string name = "oj";
};

struct JudgeConfig {
    std::string compiler = "g++";
    std::string temp_dir = "./build/judge_tmp";
    int worker_count = 2;
    int time_limit_ms = 1000;
};

struct ServerConfig {
    std::string listen_address = "0.0.0.0";
    int port = 8080;
    std::string web_root = "./web";
    std::string log_level = "info";
    DatabaseConfig database;
    JudgeConfig judge;
};

class ConfigManager {
public:
    static ConfigManager& instance();

    bool load(const std::string& path);

    const ServerConfig& config() const;

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

private:
    ConfigManager() = default;

    ServerConfig config_;
};
