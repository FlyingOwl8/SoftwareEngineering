#include "auth/jwt_utils.hpp"
#include "auth/jwt_auth_checker.hpp"
#include "auth/jwt_auth_factory.hpp"
#include "service/user_service.hpp"
#include "handlers/auth/register_handler.hpp"
#include "handlers/auth/login_handler.hpp"
#include "handlers/users/get_by_login_handler.hpp"
#include "handlers/users/search_handler.hpp"

#include "exceptions.hpp"

#include <memory>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
    const char* secret = std::getenv("JWT_SECRET");
    const char* exp    = std::getenv("JWT_EXPIRATION_HOURS");

    if (!secret) throw disk::exceptions::MissingEnvVarException("JWT_SECRET");
    if (!exp)    throw disk::exceptions::MissingEnvVarException("JWT_EXPIRATION_HOURS");

    disk::auth::InitJwt(secret, std::stoi(exp));

    auto component_list = userver::components::MinimalServerComponentList();

    component_list.Append<disk::auth::JwtAuthComponent>();
    userver::server::handlers::auth::RegisterAuthCheckerFactory(
        disk::auth::JwtAuthCheckerFactory::kAuthType,
        std::make_unique<disk::auth::JwtAuthCheckerFactory>());

    disk::user_service::AppendUserService(component_list);

    disk::handlers::auth::AppendRegisterHandler(component_list);
    disk::handlers::auth::AppendLoginHandler(component_list);
    disk::handlers::users::AppendGetByLoginHandler(component_list);
    disk::handlers::users::AppendSearchHandler(component_list);

    return userver::utils::DaemonMain(argc, argv, component_list);
}
