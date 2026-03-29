#pragma once

#include <string>
#include <chrono>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace disk::models {

using TimePoint = std::chrono::system_clock::time_point;

inline std::string FormatTimestamp(TimePoint tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
    return buf;
}

// ─── User ─────────────────────────────────────────────────────────────────────

enum class UserRole { kUser, kAdmin };

inline std::string RoleToString(UserRole role) {
    return role == UserRole::kAdmin ? "admin" : "user";
}

inline UserRole RoleFromString(const std::string& s) {
    return s == "admin" ? UserRole::kAdmin : UserRole::kUser;
}

struct User {
    std::string id;
    std::string login;
    std::string first_name;
    std::string last_name;
    std::string password_hash;
    UserRole    role{UserRole::kUser};
    TimePoint   created_at;

    userver::formats::json::Value ToJson() const {
        userver::formats::json::ValueBuilder b;
        b["id"]         = id;
        b["login"]      = login;
        b["first_name"] = first_name;
        b["last_name"]  = last_name;
        b["role"]       = RoleToString(role);
        b["created_at"] = FormatTimestamp(created_at);
        return b.ExtractValue();
    }
};

// ─── Folder ───────────────────────────────────────────────────────────────────

struct Folder {
    std::string id;
    std::string name;
    std::string owner_id;
    TimePoint   created_at;

    userver::formats::json::Value ToJson() const {
        userver::formats::json::ValueBuilder b;
        b["id"]         = id;
        b["name"]       = name;
        b["owner_id"]   = owner_id;
        b["created_at"] = FormatTimestamp(created_at);
        return b.ExtractValue();
    }
};

// ─── File ─────────────────────────────────────────────────────────────────────

struct File {
    std::string id;
    std::string name;
    std::string folder_id;
    std::string owner_id;
    std::string content_type;
    int64_t     size{0};
    std::string content;
    TimePoint   created_at;

    userver::formats::json::Value ToJson() const {
        userver::formats::json::ValueBuilder b;
        b["id"]           = id;
        b["name"]         = name;
        b["folder_id"]    = folder_id;
        b["owner_id"]     = owner_id;
        b["content_type"] = content_type;
        b["size"]         = size;
        b["content"]      = content;
        b["created_at"]   = FormatTimestamp(created_at);
        return b.ExtractValue();
    }
};

}  // namespace disk::models
