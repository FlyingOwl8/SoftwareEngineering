#include "list_folders_handler.hpp"
#include "base_handler.hpp"

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/request/request_context.hpp>

namespace disk::handlers::folders {

std::string ListFoldersHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const
{
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    const auto& payload = GetAuthPayload(context);

    auto folders = fs_service_.ListFolders(payload.user_id);

    userver::formats::json::ValueBuilder arr(
        userver::formats::common::Type::kArray);
    for (const auto& f : folders) arr.PushBack(f.ToJson());

    return userver::formats::json::ToString(arr.ExtractValue());
}

void AppendListFoldersHandler(userver::components::ComponentList& list) {
    list.Append<ListFoldersHandler>();
}

}  // namespace disk::handlers::folders
