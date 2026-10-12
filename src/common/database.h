#pragma once

#include "config.h"
#include "oj/models.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

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
    bool list_problems(std::vector<ProblemSummary>& problems);
    bool find_problem_by_id(std::uint64_t id, Problem& problem);
    bool list_sample_test_cases(std::uint64_t problem_id,
                                std::vector<TestCase>& test_cases);

private:
    void* mysql_ = nullptr;
    std::mutex mutex_;
};
