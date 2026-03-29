#pragma once

#include "i_file_system_repository.hpp"

#include <shared_mutex>
#include <unordered_map>

namespace disk::folder_file_service {

class InMemoryFileSystemRepository final : public IFileSystemRepository {
public:
    bool FolderNameExists(const std::string& owner_id,
                          const std::string& name) const override;
    void SaveFolder(const models::Folder& folder) override;
    std::optional<models::Folder> FindFolder(const std::string& folder_id) const override;
    std::vector<models::Folder>   FindFoldersByOwner(const std::string& owner_id) const override;
    bool DeleteFolderCascade(const std::string& folder_id) override;

    bool FileNameExists(const std::string& folder_id,
                        const std::string& name) const override;
    void SaveFile(const models::File& file) override;
    std::optional<models::File> FindFileByName(const std::string& folder_id,
                                               const std::string& name) const override;
    std::optional<models::File> FindFileById(const std::string& file_id) const override;
    bool DeleteFile(const std::string& file_id) override;

private:
    mutable std::shared_mutex mutex_;

    std::unordered_map<std::string, models::Folder> folders_;  // folder_id → folder
    std::unordered_map<std::string, models::File>   files_;    // file_id   → file
};

}  // namespace disk::folder_file_service
