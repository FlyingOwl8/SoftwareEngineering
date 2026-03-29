#include "get_file_handler.hpp"
#include "base_handler.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/request_context.hpp>

namespace disk::handlers::files {

std::string GetFileHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const
{
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    const auto& payload = GetAuthPayload(context);

    const auto folder_id = request.GetPathArg("folder_id");
    const auto file_name = request.GetPathArg("file_name");

    auto folder = fs_service_.FindFolder(folder_id);
    if (!folder) {
        response.SetStatus(userver::server::http::HttpStatus::kNotFound);
        return MakeError("Not found", "Folder '" + folder_id + "' not found");
    }
    if (folder->owner_id != payload.user_id) {
        response.SetStatus(userver::server::http::HttpStatus::kForbidden);
        return MakeError("Forbidden", "You are not the owner of this folder");
    }

    auto file = fs_service_.FindFileByName(folder_id, file_name);
    if (!file) {
        response.SetStatus(userver::server::http::HttpStatus::kNotFound);
        return MakeError("Not found",
                         "File '" + file_name + "' not found in folder '" + folder_id + "'");
    }

    return userver::formats::json::ToString(file->ToJson());
}

void AppendGetFileHandler(userver::components::ComponentList& list) {
    list.Append<GetFileHandler>();
}

}  // namespace disk::handlers::files
