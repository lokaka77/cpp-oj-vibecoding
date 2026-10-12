#pragma once

#include <httplib.h>

#include <nlohmann/json.hpp>

#include <string>

inline void send_json(httplib::Response& response,
                      int status,
                      const nlohmann::json& body) {
    response.status = status;
    response.set_content(body.dump(), "application/json");
}

inline void send_error(httplib::Response& response,
                       int status,
                       const std::string& code,
                       const std::string& message) {
    send_json(response, status,
              {{"error", {{"code", code}, {"message", message}}}});
}

inline void send_error(httplib::Response& response,
                       int status,
                       const std::string& code,
                       const std::string& message,
                       const nlohmann::json& details) {
    send_json(response, status,
              {{"error",
                {{"code", code},
                 {"message", message},
                 {"details", details}}}});
}

inline bool parse_json_body(const httplib::Request& request,
                            nlohmann::json& body) {
    try {
        body = nlohmann::json::parse(request.body);
        return body.is_object();
    } catch (const std::exception&) {
        return false;
    }
}

inline std::string cookie_value(const httplib::Request& request,
                                const std::string& name) {
    std::string header = request.get_header_value("Cookie");
    if (header.empty()) {
        return {};
    }

    std::size_t position = 0;
    while (position < header.size()) {
        std::size_t end = header.find(';', position);
        if (end == std::string::npos) {
            end = header.size();
        }

        std::string part = header.substr(position, end - position);
        std::size_t equals = part.find('=');
        if (equals != std::string::npos &&
            part.substr(0, equals) == name) {
            return part.substr(equals + 1);
        }

        position = end + 1;
    }

    return {};
}
