#include "user_service.hpp"
#include "repository/in_memory_user_repository.hpp"
#include "repository/postgres_user_repository.hpp"
#include "utils/password_utils.hpp"

#include "exceptions.hpp"

#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <cctype>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace disk::user_service {

UserService::UserService(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context)
{
    if (config["use_postgres"].As<bool>(false)) {
        auto& pg = context.FindComponent<userver::components::Postgres>("postgres-db");
        repository_ = std::make_unique<PostgresUserRepository>(pg.GetCluster());
    } else {
        repository_ = std::make_unique<InMemoryUserRepository>();
    }

    const char* login  = std::getenv("ADMIN_LOGIN");
    const char* pass   = std::getenv("ADMIN_PASSWORD");
    const char* fname  = std::getenv("ADMIN_FIRST_NAME");
    const char* lname  = std::getenv("ADMIN_LAST_NAME");

    if (!login) throw disk::exceptions::MissingEnvVarException("ADMIN_LOGIN");
    if (!pass)  throw disk::exceptions::MissingEnvVarException("ADMIN_PASSWORD");
    if (!fname) throw disk::exceptions::MissingEnvVarException("ADMIN_FIRST_NAME");
    if (!lname) throw disk::exceptions::MissingEnvVarException("ADMIN_LAST_NAME");

    CreateUser(login, fname, lname, utils::HashPassword(pass), models::UserRole::kAdmin);
}

std::optional<models::User> UserService::CreateUser(
    const std::string& login,
    const std::string& first_name,
    const std::string& last_name,
    const std::string& password_hash,
    models::UserRole   role)
{
    if (repository_->ExistsByLogin(login)) return std::nullopt;

    models::User user;
    user.id            = GenerateUuid();
    user.login         = login;
    user.first_name    = first_name;
    user.last_name     = last_name;
    user.password_hash = password_hash;
    user.role          = role;
    user.created_at    = std::chrono::system_clock::now();

    repository_->Save(user);
    return user;
}

std::optional<models::User> UserService::FindUserByLogin(const std::string& login) const {
    return repository_->FindByLogin(login);
}

std::vector<models::User> UserService::SearchUsers(
    const std::string& first_name_mask,
    const std::string& last_name_mask) const
{
    const auto all = repository_->FindAll();
    std::vector<models::User> result;
    for (const auto& user : all) {
        const bool match_first = first_name_mask.empty()
            || ContainsIgnoreCase(user.first_name, first_name_mask);
        const bool match_last  = last_name_mask.empty()
            || ContainsIgnoreCase(user.last_name, last_name_mask);
        if (match_first && match_last) result.push_back(user);
    }
    return result;
}

std::string UserService::GenerateUuid() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dist;

    uint64_t hi = dist(gen);
    uint64_t lo = dist(gen);
    hi = (hi & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    lo = (lo & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8)  << (hi >> 32)               << '-'
       << std::setw(4)  << ((hi >> 16) & 0xFFFF)    << '-'
       << std::setw(4)  << (hi & 0xFFFF)             << '-'
       << std::setw(4)  << (lo >> 48)                << '-'
       << std::setw(12) << (lo & 0xFFFFFFFFFFFFULL);
    return ss.str();
}

std::string UserService::ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

bool UserService::ContainsIgnoreCase(const std::string& haystack,
                                     const std::string& needle) {
    return ToLower(haystack).find(ToLower(needle)) != std::string::npos;
}

userver::yaml_config::Schema UserService::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<ComponentBase>(R"(
type: object
description: User service (in-memory or PostgreSQL)
additionalProperties: false
properties:
    use_postgres:
        type: boolean
        description: Use PostgreSQL repository instead of in-memory
)");
}

void AppendUserService(userver::components::ComponentList& component_list) {
    component_list.Append<UserService>();
}

}  // namespace disk::user_service
