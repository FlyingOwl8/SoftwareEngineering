#include "create_file_handler.hpp"
#include "base_handler.hpp"

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/request_context.hpp>

namespace disk::handlers::files {

std::string CreateFileHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const
{
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    const auto& payload = GetAuthPayload(context);

    const auto folder_id = request.GetPathArg("folder_id");

    auto folder = fs_service_.FindFolder(folder_id);
    if (!folder) {
        response.SetStatus(userver::server::http::HttpStatus::kNotFound);
        return MakeError("Not found", "Folder '" + folder_id + "' not found");
    }
    if (folder->owner_id != payload.user_id) {
        response.SetStatus(userver::server::http::HttpStatus::kForbidden);
        return MakeError("Forbidden", "You are not the owner of this folder");
    }

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

    const auto name         = body["name"].As<std::string>();
    const auto content_type = body.HasMember("content_type")
        ? body["content_type"].As<std::string>()
        : "application/octet-stream";
    const auto size = body.HasMember("size")
        ? body["size"].As<int64_t>()
        : 0LL;
    const auto content = body.HasMember("content")
        ? body["content"].As<std::string>()
        : "";

    auto file = fs_service_.CreateFile(folder_id, payload.user_id,
                                        name, content_type, size, content);
    if (!file) {
        response.SetStatus(userver::server::http::HttpStatus::kConflict);
        return MakeError("File '" + name + "' already exists in this folder");
    }

    response.SetStatus(userver::server::http::HttpStatus::kCreated);
    return userver::formats::json::ToString(file->ToJson());
}

void AppendCreateFileHandler(userver::components::ComponentList& list) {
    list.Append<CreateFileHandler>();
}

}  // namespace disk::handlers::files
