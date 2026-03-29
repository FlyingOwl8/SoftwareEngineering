#pragma once

#include "jwt_auth_checker.hpp"

#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/auth/auth_checker_settings.hpp>

namespace disk::auth {

class JwtAuthCheckerFactory final
    : public userver::server::handlers::auth::AuthCheckerFactoryBase {
public:
    static constexpr const char* kAuthType = "jwt-auth";

    userver::server::handlers::auth::AuthCheckerBasePtr operator()(
        const userver::components::ComponentContext& context,
        const userver::server::handlers::auth::HandlerAuthConfig&,
        const userver::server::handlers::auth::AuthCheckerSettings&) const override;
};

}  // namespace disk::auth
