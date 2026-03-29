#include "register_handler.hpp"
#include "base_handler.hpp"
#include "models/models.hpp"
#include "utils/password_utils.hpp"

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/request_context.hpp>

namespace disk::handlers::auth {

std::string RegisterHandler::HandleRequestThrow(
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

    if (!body.HasMember("login") || !body.HasMember("first_name") ||
        !body.HasMember("last_name") || !body.HasMember("password")) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return MakeError("Validation failed",
                         "Required fields: login, first_name, last_name, password");
    }

    const auto login      = body["login"].As<std::string>();
    const auto first_name = body["first_name"].As<std::string>();
    const auto last_name  = body["last_name"].As<std::string>();
    const auto password   = body["password"].As<std::string>();

    if (login.size() < 3) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return MakeError("Validation failed", "Login must be at least 3 characters");
    }
    if (password.size() < 6) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return MakeError("Validation failed", "Password must be at least 6 characters");
    }

    auto user = user_service_.CreateUser(login, first_name, last_name,
                                         user_service::utils::HashPassword(password),
                                         models::UserRole::kUser);
    if (!user) {
        response.SetStatus(userver::server::http::HttpStatus::kConflict);
        return MakeError("User with login '" + login + "' already exists");
    }

    response.SetStatus(userver::server::http::HttpStatus::kCreated);
    return userver::formats::json::ToString(user->ToJson());
}

void AppendRegisterHandler(userver::components::ComponentList& list) {
    list.Append<RegisterHandler>();
}

}  // namespace disk::handlers::auth
