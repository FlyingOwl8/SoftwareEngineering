#include "search_handler.hpp"
#include "base_handler.hpp"

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/request_context.hpp>

namespace disk::handlers::users {

std::string SearchHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const
{
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    // Токен уже проверен JwtChecker. Проверяем роль — поиск только для admin.
    const auto& payload = GetAuthPayload(context);
    if (!payload.IsAdmin()) {
        response.SetStatus(userver::server::http::HttpStatus::kForbidden);
        return MakeError("Forbidden",
                         "User search requires admin role. "
                         "Access denied for role '" + payload.role + "'");
    }

    const auto first_name = request.GetArg("first_name");
    const auto last_name  = request.GetArg("last_name");

    if (first_name.empty() && last_name.empty()) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return MakeError("Bad request",
                         "At least one of 'first_name' or 'last_name' query params required");
    }

    auto users = user_service_.SearchUsers(first_name, last_name);

    userver::formats::json::ValueBuilder arr(
        userver::formats::common::Type::kArray);
    for (const auto& u : users) arr.PushBack(u.ToJson());

    return userver::formats::json::ToString(arr.ExtractValue());
}

void AppendSearchHandler(userver::components::ComponentList& list) {
    list.Append<SearchHandler>();
}

}  // namespace disk::handlers::users
