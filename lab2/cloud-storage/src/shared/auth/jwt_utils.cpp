#include "jwt_utils.hpp"

#include <chrono>

#include <jwt-cpp/jwt.h>

namespace disk::auth {

namespace {
static std::string g_secret;
static int         g_expiration_hours = 24;
}  // namespace

void InitJwt(const std::string& secret, int expiration_hours) {
    g_secret           = secret;
    g_expiration_hours = expiration_hours;
}

std::string GenerateToken(const std::string& user_id,
                          const std::string& login,
                          const std::string& role) {
    auto now = std::chrono::system_clock::now();
    return jwt::create()
        .set_type("JWT")
        .set_subject(user_id)
        .set_payload_claim("login", jwt::claim(login))
        .set_payload_claim("role",  jwt::claim(role))
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::hours(g_expiration_hours))
        .sign(jwt::algorithm::hs256{g_secret});
}

}  // namespace disk::auth
