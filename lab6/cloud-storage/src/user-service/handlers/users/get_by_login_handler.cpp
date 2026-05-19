#include "get_by_login_handler.hpp"
#include "base_handler.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/request_context.hpp>

namespace disk::handlers::users {

std::string GetByLoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const
{
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    // Токен уже проверен JwtChecker до вызова хендлера.
    // Проверяем роль — поиск пользователей только для администратора.
    const auto& payload = GetAuthPayload(context);
    if (!payload.IsAdmin()) {
        response.SetStatus(userver::server::http::HttpStatus::kForbidden);
        return MakeError("Forbidden", "Admin role required");
    }

    const auto login = request.GetPathArg("login");
    if (login.empty()) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return MakeError("Bad request", "Login path parameter is required");
    }

    auto user = user_service_.FindUserByLogin(login);
    if (!user) {
        response.SetStatus(userver::server::http::HttpStatus::kNotFound);
        return MakeError("Not found", "User with login '" + login + "' not found");
    }

    return userver::formats::json::ToString(user->ToJson());
}

void AppendGetByLoginHandler(userver::components::ComponentList& list) {
    list.Append<GetByLoginHandler>();
}

}  // namespace disk::handlers::users
