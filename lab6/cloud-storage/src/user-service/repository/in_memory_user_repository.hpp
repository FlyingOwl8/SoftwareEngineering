#pragma once

#include "i_user_repository.hpp"

#include <shared_mutex>
#include <unordered_map>

namespace disk::user_service {

class InMemoryUserRepository final : public IUserRepository {
public:
    bool ExistsByLogin(const std::string& login) const override;
    void Save(const models::User& user) override;
    std::optional<models::User> FindByLogin(const std::string& login) const override;
    std::vector<models::User>   FindAll() const override;

private:
    mutable std::shared_mutex mutex_;

    std::unordered_map<std::string, models::User> users_by_id_;
    std::unordered_map<std::string, std::string>  login_to_id_;  // login → user_id
};

}  // namespace disk::user_service
