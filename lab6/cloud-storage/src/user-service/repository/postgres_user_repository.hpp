#pragma once

#include "i_user_repository.hpp"

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/result_set.hpp>

namespace disk::user_service {

class PostgresUserRepository final : public IUserRepository {
public:
    explicit PostgresUserRepository(userver::storages::postgres::ClusterPtr pg);

    bool ExistsByLogin(const std::string& login) const override;
    void Save(const models::User& user) override;
    std::optional<models::User> FindByLogin(const std::string& login) const override;
    std::vector<models::User> FindAll() const override;

private:
    userver::storages::postgres::ClusterPtr pg_;

    static models::User RowToUser(const userver::storages::postgres::Row& row);
};

}  // namespace disk::user_service
