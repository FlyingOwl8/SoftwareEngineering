#include "in_memory_user_repository.hpp"

#include <mutex>

namespace disk::user_service {

bool InMemoryUserRepository::ExistsByLogin(const std::string& login) const {
    std::shared_lock lock(mutex_);
    return login_to_id_.count(login) > 0;
}

void InMemoryUserRepository::Save(const models::User& user) {
    std::unique_lock lock(mutex_);
    users_by_id_[user.id]    = user;
    login_to_id_[user.login] = user.id;
}

std::optional<models::User> InMemoryUserRepository::FindByLogin(const std::string& login) const {
    std::shared_lock lock(mutex_);
    auto it = login_to_id_.find(login);
    if (it == login_to_id_.end()) return std::nullopt;
    return users_by_id_.at(it->second);
}

std::vector<models::User> InMemoryUserRepository::FindAll() const {
    std::shared_lock lock(mutex_);
    std::vector<models::User> result;
    result.reserve(users_by_id_.size());
    for (const auto& [id, user] : users_by_id_) {
        result.push_back(user);
    }
    return result;
}

}  // namespace disk::user_service
