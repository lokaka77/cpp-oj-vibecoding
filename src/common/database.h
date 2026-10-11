#pragma once

#include "config.h"

#include <cstdint>
#include <mutex>
#include <string>

struct AuthUser {
    std::uint64_t id = 0;
    std::string username;
    std::string password_hash;
    std::string role;
};

class Database {
public:
    ~Database();

    bool connect(const DatabaseConfig& config);

    bool find_user_by_username(const std::string& username, AuthUser& user);
    bool find_user_by_id(std::uint64_t id, AuthUser& user);
    bool insert_user(const std::string& username,
                     const std::string& password_hash,
                     const std::string& role);

private:
    void* mysql_ = nullptr;
    std::mutex mutex_;
};
