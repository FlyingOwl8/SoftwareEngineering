#include "jwt_auth_checker.hpp"

#include "exceptions.hpp"

#include <cstdlib>

#include <jwt-cpp/jwt.h>

#include <userver/http/common_headers.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace disk::auth {

namespace {
constexpr std::string_view kBearerPrefix = "Bearer ";
}

// ─── JwtChecker ───────────────────────────────────────────────────────────────

JwtChecker::JwtChecker(std::string secret) : secret_(std::move(secret)) {}

JwtChecker::AuthCheckResult JwtChecker::CheckAuth(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const
{
    const auto auth_header =
        std::string(request.GetHeader(userver::http::headers::kAuthorization));

    if (auth_header.empty() ||
        auth_header.size() <= kBearerPrefix.size() ||
        auth_header.substr(0, kBearerPrefix.size()) != kBearerPrefix) {
        return AuthCheckResult{AuthCheckResult::Status::kInvalidToken,
                               "Missing or malformed 'Authorization: Bearer <token>' header"};
    }

    const std::string token = auth_header.substr(kBearerPrefix.size());

    try {
        auto decoded = jwt::decode(token);

        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{secret_})
            .verify(decoded);

        JwtPayload payload;
        payload.user_id = decoded.get_subject();
        payload.login   = decoded.get_payload_claim("login").as_string();
        payload.role    = decoded.get_payload_claim("role").as_string();

        if (payload.user_id.empty()) {
            return AuthCheckResult{AuthCheckResult::Status::kInvalidToken,
                                   "Token missing 'sub' claim"};
        }

        context.EmplaceData<JwtPayload>(std::string(kPayloadKey),
                                        std::move(payload));
        return {};

    } catch (const jwt::error::token_verification_exception& ex) {
        return AuthCheckResult{AuthCheckResult::Status::kInvalidToken,
                               std::string("Token verification failed: ") + ex.what()};
    } catch (const std::exception& ex) {
        return AuthCheckResult{AuthCheckResult::Status::kForbidden,
                               std::string("Token processing error: ") + ex.what()};
    }
}

// ─── JwtAuthComponent ─────────────────────────────────────────────────────────

JwtAuthComponent::JwtAuthComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : LoggableComponentBase(config, context)
{
    const char* secret_env = std::getenv("JWT_SECRET");
    if (!secret_env) throw disk::exceptions::MissingEnvVarException("JWT_SECRET");

    checker_ = std::make_shared<JwtChecker>(secret_env);
}

JwtCheckerPtr JwtAuthComponent::Get() const { return checker_; }

userver::yaml_config::Schema JwtAuthComponent::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: JWT Auth Checker Component (reads JWT_SECRET env var)
additionalProperties: false
properties: {}
)");
}

}  // namespace disk::auth
