#include "config.h"

#include <yaml-cpp/yaml.h>

#include <stdexcept>

ConfigManager& ConfigManager::instance() {
    static ConfigManager manager;
    return manager;
}

bool ConfigManager::load(const std::string& path) {
    try {
        config_ = ServerConfig{};

        YAML::Node root = YAML::LoadFile(path);

        YAML::Node server = root["server"];
        if (!server) {
            throw std::runtime_error("Missing 'server' section in config file");
        }

        if (server["listen_address"]) {
            config_.listen_address =
                server["listen_address"].as<std::string>();
        }

        if (server["port"]) {
            config_.port = server["port"].as<int>();
        }

        if (server["web_root"]) {
            config_.web_root = server["web_root"].as<std::string>();
        }

        YAML::Node logging = root["logging"];
        if (logging && logging["level"]) {
            config_.log_level = logging["level"].as<std::string>();
        }

        if (config_.port < 1 || config_.port > 65535) {
            throw std::runtime_error("Invalid server port in config file");
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

const ServerConfig& ConfigManager::config() const {
    return config_;
}
