#include "mongo_file_system_repository.hpp"

#include <userver/formats/bson/value_builder.hpp>
#include <userver/storages/mongo/options.hpp>

namespace disk::folder_file_service {

namespace bson  = userver::formats::bson;
namespace mongo = userver::storages::mongo;

static constexpr std::string_view kFolders = "folders";
static constexpr std::string_view kFiles   = "files";

// Helper: build a BSON document from a list of key-value pairs at compile time.
// Usage: MakeDoc("k1", v1, "k2", v2, ...)
namespace {
void FillDoc(bson::ValueBuilder&) {}

template <typename V, typename... Rest>
void FillDoc(bson::ValueBuilder& b,
             std::string_view key, V&& val,
             Rest&&... rest)
{
    b[std::string{key}] = std::forward<V>(val);
    FillDoc(b, std::forward<Rest>(rest)...);
}

template <typename... Args>
bson::Value MakeDoc(Args&&... args)
{
    static_assert(sizeof...(args) % 2 == 0, "Expected key-value pairs");
    bson::ValueBuilder b(bson::ValueBuilder::Type::kObject);
    FillDoc(b, std::forward<Args>(args)...);
    return b.ExtractValue();
}
} // namespace

MongoFileSystemRepository::MongoFileSystemRepository(mongo::PoolPtr pool)
    : pool_(std::move(pool)) {}

// ── Папки ─────────────────────────────────────────────────────────────────────

bool MongoFileSystemRepository::FolderNameExists(
    const std::string& owner_id, const std::string& name) const
{
    auto coll = pool_->GetCollection(std::string{kFolders});
    auto doc = coll.FindOne(MakeDoc("owner_id", owner_id, "name", name));
    return doc.has_value();
}

void MongoFileSystemRepository::SaveFolder(const models::Folder& folder) {
    auto coll = pool_->GetCollection(std::string{kFolders});
    bson::ValueBuilder doc(bson::ValueBuilder::Type::kObject);
    doc["_id"]        = folder.id;
    doc["name"]       = folder.name;
    doc["owner_id"]   = folder.owner_id;
    doc["created_at"] = folder.created_at;
    coll.InsertOne(doc.ExtractValue());
}

std::optional<models::Folder> MongoFileSystemRepository::FindFolder(
    const std::string& folder_id) const
{
    auto coll = pool_->GetCollection(std::string{kFolders});
    auto doc = coll.FindOne(MakeDoc("_id", folder_id));
    if (!doc) return std::nullopt;
    return DocToFolder(*doc);
}

std::vector<models::Folder> MongoFileSystemRepository::FindFoldersByOwner(
    const std::string& owner_id) const
{
    auto coll   = pool_->GetCollection(std::string{kFolders});
    auto cursor = coll.Find(MakeDoc("owner_id", owner_id),
                            mongo::options::Sort{{"created_at",
                                mongo::options::Sort::Direction::kAscending}});
    std::vector<models::Folder> result;
    for (const auto& doc : cursor) {
        result.push_back(DocToFolder(doc));
    }
    return result;
}

bool MongoFileSystemRepository::DeleteFolderCascade(const std::string& folder_id) {
    auto files_coll   = pool_->GetCollection(std::string{kFiles});
    files_coll.DeleteMany(MakeDoc("folder_id", folder_id));

    auto folders_coll = pool_->GetCollection(std::string{kFolders});
    auto result       = folders_coll.DeleteOne(MakeDoc("_id", folder_id));
    return result.DeletedCount() > 0;
}

// ── Файлы ─────────────────────────────────────────────────────────────────────

bool MongoFileSystemRepository::FileNameExists(
    const std::string& folder_id, const std::string& name) const
{
    auto coll = pool_->GetCollection(std::string{kFiles});
    auto doc  = coll.FindOne(MakeDoc("folder_id", folder_id, "name", name));
    return doc.has_value();
}

void MongoFileSystemRepository::SaveFile(const models::File& file) {
    auto coll = pool_->GetCollection(std::string{kFiles});
    bson::ValueBuilder doc(bson::ValueBuilder::Type::kObject);
    doc["_id"]          = file.id;
    doc["name"]         = file.name;
    doc["folder_id"]    = file.folder_id;
    doc["owner_id"]     = file.owner_id;
    doc["content_type"] = file.content_type;
    doc["size"]         = file.size;
    doc["content"]      = file.content;
    doc["created_at"]   = file.created_at;
    coll.InsertOne(doc.ExtractValue());
}

std::optional<models::File> MongoFileSystemRepository::FindFileByName(
    const std::string& folder_id, const std::string& name) const
{
    auto coll = pool_->GetCollection(std::string{kFiles});
    auto doc  = coll.FindOne(MakeDoc("folder_id", folder_id, "name", name));
    if (!doc) return std::nullopt;
    return DocToFile(*doc);
}

std::optional<models::File> MongoFileSystemRepository::FindFileById(
    const std::string& file_id) const
{
    auto coll = pool_->GetCollection(std::string{kFiles});
    auto doc  = coll.FindOne(MakeDoc("_id", file_id));
    if (!doc) return std::nullopt;
    return DocToFile(*doc);
}

bool MongoFileSystemRepository::DeleteFile(const std::string& file_id) {
    auto coll   = pool_->GetCollection(std::string{kFiles});
    auto result = coll.DeleteOne(MakeDoc("_id", file_id));
    return result.DeletedCount() > 0;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

models::Folder MongoFileSystemRepository::DocToFolder(const bson::Document& doc) {
    models::Folder folder;
    folder.id         = doc["_id"].As<std::string>();
    folder.name       = doc["name"].As<std::string>();
    folder.owner_id   = doc["owner_id"].As<std::string>();
    folder.created_at = doc["created_at"].As<std::chrono::system_clock::time_point>();
    return folder;
}

models::File MongoFileSystemRepository::DocToFile(const bson::Document& doc) {
    models::File file;
    file.id           = doc["_id"].As<std::string>();
    file.name         = doc["name"].As<std::string>();
    file.folder_id    = doc["folder_id"].As<std::string>();
    file.owner_id     = doc["owner_id"].As<std::string>();
    file.content_type = doc["content_type"].As<std::string>();
    file.size         = doc["size"].As<int64_t>();
    file.content      = doc["content"].As<std::string>();
    file.created_at   = doc["created_at"].As<std::chrono::system_clock::time_point>();
    return file;
}

}  // namespace disk::folder_file_service
