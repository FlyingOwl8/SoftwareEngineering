#include "file_system_service.hpp"
#include "repository/in_memory_file_system_repository.hpp"
#include "repository/mongo_file_system_repository.hpp"

#include <random>
#include <sstream>
#include <iomanip>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/mongo/component.hpp>

namespace disk::folder_file_service {

FileSystemService::FileSystemService(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context)
{
    if (config["use_mongo"].As<bool>(false)) {
        auto& mongo = context.FindComponent<userver::components::Mongo>("mongo-db");
        repository_ = std::make_unique<MongoFileSystemRepository>(mongo.GetPool());
    } else {
        repository_ = std::make_unique<InMemoryFileSystemRepository>();
    }
}

std::optional<models::Folder> FileSystemService::CreateFolder(
    const std::string& name,
    const std::string& owner_id)
{
    if (repository_->FolderNameExists(owner_id, name)) return std::nullopt;

    models::Folder folder;
    folder.id         = GenerateUuid();
    folder.name       = name;
    folder.owner_id   = owner_id;
    folder.created_at = std::chrono::system_clock::now();

    repository_->SaveFolder(folder);
    return folder;
}

std::vector<models::Folder> FileSystemService::ListFolders(const std::string& owner_id) const {
    return repository_->FindFoldersByOwner(owner_id);
}

std::optional<models::Folder> FileSystemService::FindFolder(const std::string& folder_id) const {
    return repository_->FindFolder(folder_id);
}

bool FileSystemService::DeleteFolder(const std::string& folder_id) {
    return repository_->DeleteFolderCascade(folder_id);
}

std::optional<models::File> FileSystemService::CreateFile(
    const std::string& folder_id,
    const std::string& owner_id,
    const std::string& name,
    const std::string& content_type,
    int64_t            size,
    const std::string& content)
{
    if (repository_->FileNameExists(folder_id, name)) return std::nullopt;

    models::File file;
    file.id           = GenerateUuid();
    file.name         = name;
    file.folder_id    = folder_id;
    file.owner_id     = owner_id;
    file.content_type = content_type.empty() ? "application/octet-stream" : content_type;
    file.size         = size;
    file.content      = content;
    file.created_at   = std::chrono::system_clock::now();

    repository_->SaveFile(file);
    return file;
}

std::optional<models::File> FileSystemService::FindFileByName(
    const std::string& folder_id,
    const std::string& name) const
{
    return repository_->FindFileByName(folder_id, name);
}

std::optional<models::File> FileSystemService::FindFileById(const std::string& file_id) const {
    return repository_->FindFileById(file_id);
}

bool FileSystemService::DeleteFile(const std::string& file_id) {
    return repository_->DeleteFile(file_id);
}

std::string FileSystemService::GenerateUuid() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dist;

    uint64_t hi = dist(gen);
    uint64_t lo = dist(gen);
    hi = (hi & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    lo = (lo & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8)  << (hi >> 32)               << '-'
       << std::setw(4)  << ((hi >> 16) & 0xFFFF)    << '-'
       << std::setw(4)  << (hi & 0xFFFF)             << '-'
       << std::setw(4)  << (lo >> 48)                << '-'
       << std::setw(12) << (lo & 0xFFFFFFFFFFFFULL);
    return ss.str();
}

userver::yaml_config::Schema FileSystemService::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<ComponentBase>(R"(
type: object
description: File system service (in-memory or MongoDB)
additionalProperties: false
properties:
    use_mongo:
        type: boolean
        description: Use MongoDB repository instead of in-memory
)");
}

void AppendFileSystemService(userver::components::ComponentList& component_list) {
    component_list.Append<FileSystemService>();
}

}  // namespace disk::folder_file_service
