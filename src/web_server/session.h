#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

class SessionManager {
public:
    std::string create(std::uint64_t user_id);
    bool get(const std::string& token, std::uint64_t& user_id) const;
    void remove(const std::string& token);

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::uint64_t> sessions_;
};
