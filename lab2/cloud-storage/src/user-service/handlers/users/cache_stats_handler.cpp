#include "cache_stats_handler.hpp"

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/request/request_context.hpp>

namespace disk::handlers::users {

std::string CacheStatsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const
{
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    const auto stats = user_service_.GetUserCacheStats();

    userver::formats::json::ValueBuilder doc;
    doc["cache"] = "user_by_login";
    doc["hits"]  = stats.hits;
    doc["misses"] = stats.misses;
    doc["hit_rate"] = stats.hit_rate;

    return userver::formats::json::ToString(doc.ExtractValue());
}

void AppendCacheStatsHandler(userver::components::ComponentList& list) {
    list.Append<CacheStatsHandler>();
}

}  // namespace disk::handlers::users
