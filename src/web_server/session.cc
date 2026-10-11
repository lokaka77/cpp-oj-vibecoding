#include "session.h"

#include <sodium.h>

namespace {

std::string bytes_to_hex(const unsigned char* bytes, std::size_t size) {
    static const char* digits = "0123456789abcdef";
    std::string output;
    output.reserve(size * 2);
    for (std::size_t i = 0; i < size; ++i) {
        output.push_back(digits[(bytes[i] >> 4) & 0x0F]);
        output.push_back(digits[bytes[i] & 0x0F]);
    }
    return output;
}

}  // namespace

std::string SessionManager::create(std::uint64_t user_id) {
    if (sodium_init() < 0) {
        return {};
    }

    unsigned char random_bytes[32]{};
    randombytes_buf(random_bytes, sizeof(random_bytes));
    std::string token = bytes_to_hex(random_bytes, sizeof(random_bytes));

    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[token] = user_id;
    return token;
}

bool SessionManager::get(const std::string& token,
                         std::uint64_t& user_id) const {
    if (token.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto iterator = sessions_.find(token);
    if (iterator == sessions_.end()) {
        return false;
    }

    user_id = iterator->second;
    return true;
}

void SessionManager::remove(const std::string& token) {
    if (token.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(token);
}
