#pragma once

#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <string>

namespace disk::user_service::utils {

inline std::string HashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.data()),
           password.size(), hash);
    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    return ss.str();
}

}  // namespace disk::user_service::utils
