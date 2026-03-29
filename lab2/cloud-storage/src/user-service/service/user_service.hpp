#pragma once

#include "repository/i_user_repository.hpp"
#include "models/models.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_list.hpp>

namespace disk::user_service {

class UserService final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "user-service-logic";

    UserService(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context);

    std::optional<models::User> CreateUser(const std::string& login,
                                           const std::string& first_name,
                                           const std::string& last_name,
                                           const std::string& password_hash,
                                           models::UserRole   role = models::UserRole::kUser);

    std::optional<models::User> FindUserByLogin(const std::string& login) const;

    std::vector<models::User> SearchUsers(const std::string& first_name_mask,
                                          const std::string& last_name_mask) const;

private:
    std::unique_ptr<IUserRepository> repository_;

    static std::string GenerateUuid();
    static std::string ToLower(std::string s);
    static bool ContainsIgnoreCase(const std::string& haystack,
                                   const std::string& needle);
};

void AppendUserService(userver::components::ComponentList& component_list);

}  // namespace disk::user_service
