#include "login_handler.hpp"
#include "base_handler.hpp"
#include "models/models.hpp"
#include "auth/jwt_utils.hpp"
#include "utils/password_utils.hpp"

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/request_context.hpp>

namespace disk::handlers::auth {

std::string LoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const
{
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    userver::formats::json::Value body;
    try {
        body = userver::formats::json::FromString(request.RequestBody());
    } catch (...) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return MakeError("Bad request", "Invalid JSON body");
    }

    if (!body.HasMember("login") || !body.HasMember("password")) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return MakeError("Validation failed", "Required fields: login, password");
    }

    const auto login    = body["login"].As<std::string>();
    const auto password = body["password"].As<std::string>();

    auto user_opt = user_service_.FindUserByLogin(login);
    if (!user_opt || user_opt->password_hash != user_service::utils::HashPassword(password)) {
        response.SetStatus(userver::server::http::HttpStatus::kUnauthorized);
        return MakeError("Unauthorized", "Invalid login or password");
    }

    const auto& user  = *user_opt;
    std::string token = disk::auth::GenerateToken(
        user.id, user.login, models::RoleToString(user.role));

    userver::formats::json::ValueBuilder b;
    b["token"] = token;
    b["user"]  = user.ToJson();

    return userver::formats::json::ToString(b.ExtractValue());
}

void AppendLoginHandler(userver::components::ComponentList& list) {
    list.Append<LoginHandler>();
}

}  // namespace disk::handlers::auth
