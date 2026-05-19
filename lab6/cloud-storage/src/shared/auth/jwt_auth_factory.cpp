#include "jwt_auth_factory.hpp"

namespace disk::auth {

userver::server::handlers::auth::AuthCheckerBasePtr
JwtAuthCheckerFactory::operator()(
    const userver::components::ComponentContext& context,
    const userver::server::handlers::auth::HandlerAuthConfig&,
    const userver::server::handlers::auth::AuthCheckerSettings&) const
{
    return context.FindComponent<JwtAuthComponent>().Get();
}

}  // namespace disk::auth
