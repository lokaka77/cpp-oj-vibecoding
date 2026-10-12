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

        YAML::Node database = root["database"];
        if (database) {
            if (database["host"]) {
                config_.database.host = database["host"].as<std::string>();
            }
            if (database["port"]) {
                config_.database.port = database["port"].as<int>();
            }
            if (database["user"]) {
                config_.database.user = database["user"].as<std::string>();
            }
            if (database["password"]) {
                config_.database.password =
                    database["password"].as<std::string>();
            }
            if (database["name"]) {
                config_.database.name = database["name"].as<std::string>();
            }
        }

        YAML::Node judge = root["judge"];
        if (judge) {
            if (judge["compiler"]) {
                config_.judge.compiler = judge["compiler"].as<std::string>();
            }
            if (judge["temp_dir"]) {
                config_.judge.temp_dir = judge["temp_dir"].as<std::string>();
            }
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
