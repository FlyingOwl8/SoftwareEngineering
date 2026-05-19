#include "create_folder_handler.hpp"
#include "base_handler.hpp"

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/request_context.hpp>

namespace disk::handlers::folders {

std::string CreateFolderHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const
{
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    const auto& payload = GetAuthPayload(context);

    userver::formats::json::Value body;
    try {
        body = userver::formats::json::FromString(request.RequestBody());
    } catch (...) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return MakeError("Bad request", "Invalid JSON body");
    }

    if (!body.HasMember("name")) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return MakeError("Validation failed", "Field 'name' is required");
    }

    const auto name = body["name"].As<std::string>();
    if (name.empty()) {
        response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return MakeError("Validation failed", "Folder name cannot be empty");
    }

    auto folder = fs_service_.CreateFolder(name, payload.user_id);
    if (!folder) {
        response.SetStatus(userver::server::http::HttpStatus::kConflict);
        return MakeError("Folder '" + name + "' already exists");
    }

    response.SetStatus(userver::server::http::HttpStatus::kCreated);
    return userver::formats::json::ToString(folder->ToJson());
}

void AppendCreateFolderHandler(userver::components::ComponentList& list) {
    list.Append<CreateFolderHandler>();
}

}  // namespace disk::handlers::folders
