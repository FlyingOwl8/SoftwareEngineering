#pragma once

#include "repository/i_file_system_repository.hpp"
#include "models/models.hpp"
#include "cache/ttl_cache.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_list.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/kafka/producer.hpp>
#include <userver/kafka/producer_component.hpp>

namespace disk::folder_file_service {

class FileSystemService final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "file-system-service";

    FileSystemService(const userver::components::ComponentConfig& config,
                      const userver::components::ComponentContext& context);

    static userver::yaml_config::Schema GetStaticConfigSchema();


    std::optional<models::Folder> CreateFolder(const std::string& name,
                                               const std::string& owner_id);

    std::vector<models::Folder> ListFolders(const std::string& owner_id) const;
    std::optional<models::Folder> FindFolder(const std::string& folder_id) const;

    bool DeleteFolder(const std::string& folder_id);


    std::optional<models::File> CreateFile(const std::string& folder_id,
                                           const std::string& owner_id,
                                           const std::string& name,
                                           const std::string& content_type,
                                           int64_t            size,
                                           const std::string& content);

    std::optional<models::File> FindFileByName(const std::string& folder_id,
                                               const std::string& name) const;

    std::optional<models::File> FindFileById(const std::string& file_id) const;

    bool DeleteFile(const std::string& file_id);

    TtlCache<std::string, std::vector<models::Folder>>::Stats GetFolderCacheStats() const {
        return folders_cache_.GetStats();
    }

private:
    std::unique_ptr<IFileSystemRepository> repository_;
    mutable TtlCache<std::string, std::vector<models::Folder>> folders_cache_{std::chrono::seconds(30)};
    const userver::kafka::Producer* producer_{nullptr};

    static std::string GenerateUuid();
    static std::string FolderToKafkaPayload(const models::Folder& folder);
};

void AppendFileSystemService(userver::components::ComponentList& component_list);

}  // namespace disk::folder_file_service
