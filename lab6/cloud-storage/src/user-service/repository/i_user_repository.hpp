#pragma once

#include "models/models.hpp"

#include <optional>
#include <string>
#include <vector>

namespace disk::user_service {

class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    virtual bool ExistsByLogin(const std::string& login) const = 0;

    virtual void Save(const models::User& user) = 0;

    virtual std::optional<models::User> FindByLogin(const std::string& login) const = 0;

    virtual std::vector<models::User> FindAll() const = 0;
};

}  // namespace disk::user_service
