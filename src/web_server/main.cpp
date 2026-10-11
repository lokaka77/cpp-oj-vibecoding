#include "http_service.h"

#include <httplib.h>

#include <iostream>
#include <stdexcept>

void run_http_server(const std::string& web_root, int port) {
    httplib::Server server;

    server.set_mount_point("/", web_root);

    server.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    if (!server.listen("0.0.0.0", port)) {
        throw std::runtime_error("HTTP server failed to start");
    }
}

int main() {
    try {
        run_http_server("./web", 8080);
    } catch (const std::exception& error) {
        std::cerr << "Server error: " << error.what() << '\n';
        return 1;
    }
}
