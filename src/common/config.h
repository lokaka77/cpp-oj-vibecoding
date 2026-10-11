#pragma once

#include <string>

struct ServerConfig {
    std::string listen_address = "0.0.0.0";
    int port = 8080;
    std::string web_root = "./web";
    std::string log_level = "info";
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
