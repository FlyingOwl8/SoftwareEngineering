#pragma once

#include "jwt_utils.hpp"

#include <memory>

#include <userver/components/loggable_component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/yaml_config/schema.hpp>

namespace disk::auth {

class JwtChecker final
    : public userver::server::handlers::auth::AuthCheckerBase {
public:
    using AuthCheckResult =
        userver::server::handlers::auth::AuthCheckResult;

    explicit JwtChecker(std::string secret);

    AuthCheckResult CheckAuth(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

    bool SupportsUserAuth() const noexcept override { return true; }

    static constexpr std::string_view kPayloadKey = "jwt_payload";

private:
    std::string secret_;
};

using JwtCheckerPtr = std::shared_ptr<JwtChecker>;

class JwtAuthComponent final
    : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "jwt-auth-checker";

    JwtAuthComponent(const userver::components::ComponentConfig& config,
                     const userver::components::ComponentContext& context);

    JwtCheckerPtr Get() const;

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    JwtCheckerPtr checker_;
};

}  // namespace disk::auth
