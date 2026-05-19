#pragma once

#include <string>

namespace disk::auth {

struct JwtPayload {
    std::string user_id;
    std::string login;
    std::string role;

    bool IsAdmin() const { return role == "admin"; }
};

void InitJwt(const std::string& secret, int expiration_hours);

std::string GenerateToken(const std::string& user_id,
                          const std::string& login,
                          const std::string& role);

}  // namespace disk::auth
