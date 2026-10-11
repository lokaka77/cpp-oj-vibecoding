#include "http_service.h"
#include "logging.h"

#include <httplib.h>

#include <iostream>
#include <stdexcept>

void run_http_server(const ServerConfig& config) {
    httplib::Server server;

    server.set_mount_point("/", config.web_root);

    server.set_logger([](const httplib::Request& request,
                         const httplib::Response& response) {
        log_info(request.method + " " + request.path + " -> " +
                 std::to_string(response.status));
    });

    server.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    log_info("HTTP server listening on " + config.listen_address + ":" +
             std::to_string(config.port));

    if (!server.listen(config.listen_address, config.port)) {
        throw std::runtime_error("HTTP server failed to start");
    }
}

int main() {
    try {
        ConfigManager& config_manager = ConfigManager::instance();
        if (!config_manager.load("./config.yaml")) {
            log_error("Failed to load ./config.yaml");
            return 1;
        }

        ServerConfig config = config_manager.config();
        g_log_level = parse_log_level(config.log_level);
        run_http_server(config);
    } catch (const std::exception& error) {
        log_error(std::string("Server error: ") + error.what());
        return 1;
    }
}
