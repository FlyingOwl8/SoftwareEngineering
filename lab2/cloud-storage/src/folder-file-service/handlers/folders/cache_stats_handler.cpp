#include "cache_stats_handler.hpp"

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/request/request_context.hpp>

namespace disk::handlers::folders {

std::string CacheStatsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const
{
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    const auto stats = fs_service_.GetFolderCacheStats();

    userver::formats::json::ValueBuilder doc;
    doc["cache"]    = "folders_by_owner";
    doc["hits"]     = stats.hits;
    doc["misses"]   = stats.misses;
    doc["hit_rate"] = stats.hit_rate;

    return userver::formats::json::ToString(doc.ExtractValue());
}

void AppendFolderCacheStatsHandler(userver::components::ComponentList& list) {
    list.Append<CacheStatsHandler>();
}

}  // namespace disk::handlers::folders
