#include "postgres_user_repository.hpp"

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/io/chrono.hpp>

namespace disk::user_service {

namespace pg = userver::storages::postgres;

PostgresUserRepository::PostgresUserRepository(pg::ClusterPtr pg)
    : pg_(std::move(pg)) {}

bool PostgresUserRepository::ExistsByLogin(const std::string& login) const {
    auto res = pg_->Execute(
        pg::ClusterHostType::kMaster,
        "SELECT COUNT(*) FROM users WHERE login = $1",
        login);
    return res.Front()[0].As<int64_t>() > 0;
}

void PostgresUserRepository::Save(const models::User& user) {
    pg_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO users (id, login, first_name, last_name, password_hash, role) "
        "VALUES ($1::uuid, $2, $3, $4, $5, $6) "
        "ON CONFLICT (login) DO NOTHING",
        user.id, user.login, user.first_name, user.last_name,
        user.password_hash, models::RoleToString(user.role));
}

std::optional<models::User> PostgresUserRepository::FindByLogin(const std::string& login) const {
    auto res = pg_->Execute(
        pg::ClusterHostType::kMaster,
        "SELECT id::text, login, first_name, last_name, "
        "       rtrim(password_hash) AS password_hash, role, created_at "
        "FROM users WHERE login = $1",
        login);
    if (res.IsEmpty()) return std::nullopt;
    return RowToUser(res.Front());
}

std::vector<models::User> PostgresUserRepository::FindAll() const {
    auto res = pg_->Execute(
        pg::ClusterHostType::kMaster,
        "SELECT id::text, login, first_name, last_name, "
        "       rtrim(password_hash) AS password_hash, role, created_at "
        "FROM users ORDER BY created_at");
    std::vector<models::User> users;
    users.reserve(res.Size());
    for (const auto& row : res) {
        users.push_back(RowToUser(row));
    }
    return users;
}

models::User PostgresUserRepository::RowToUser(const pg::Row& row) {
    models::User user;
    user.id            = row["id"].As<std::string>();
    user.login         = row["login"].As<std::string>();
    user.first_name    = row["first_name"].As<std::string>();
    user.last_name     = row["last_name"].As<std::string>();
    user.password_hash = row["password_hash"].As<std::string>();
    user.role          = models::RoleFromString(row["role"].As<std::string>());
    user.created_at    = row["created_at"].As<pg::TimePointTz>().GetUnderlying();
    return user;
}

}  // namespace disk::user_service
