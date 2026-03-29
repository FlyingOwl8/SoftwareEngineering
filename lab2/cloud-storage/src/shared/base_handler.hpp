#pragma once

#include "auth/jwt_auth_checker.hpp"

#include <string>

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

namespace disk::handlers {

// ─── JSON helpers ──────────────────────────────────────────────────────────────

inline std::string MakeError(const std::string& message,
                              const std::string& details = {}) {
    userver::formats::json::ValueBuilder b;
    b["error"] = message;
    if (!details.empty()) b["details"] = details;
    return userver::formats::json::ToString(b.ExtractValue());
}

// ─── Получить payload аутентифицированного пользователя ───────────────────────

inline const disk::auth::JwtPayload& GetAuthPayload(
    userver::server::request::RequestContext& context)
{
    return context.GetData<disk::auth::JwtPayload>(
        std::string(disk::auth::JwtChecker::kPayloadKey));
}

}  // namespace disk::handlers
