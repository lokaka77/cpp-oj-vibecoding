#include "password.h"

#include <sodium.h>

std::string hash_password(const std::string& password) {
    if (sodium_init() < 0) {
        return {};
    }

    char hash[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(
            hash,
            password.c_str(),
            password.size(),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        return {};
    }

    return hash;
}

bool verify_password(const std::string& hash, const std::string& password) {
    if (hash.empty() || sodium_init() < 0) {
        return false;
    }

    return crypto_pwhash_str_verify(hash.c_str(), password.c_str(),
                                    password.size()) == 0;
}
