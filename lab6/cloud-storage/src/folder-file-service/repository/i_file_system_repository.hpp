#pragma once

#include "models/models.hpp"

#include <optional>
#include <string>
#include <vector>

namespace disk::folder_file_service {

class IFileSystemRepository {
public:
    virtual ~IFileSystemRepository() = default;

    // ── Папки ─────────────────────────────────────────────────────────────────

    virtual bool FolderNameExists(const std::string& owner_id,
                                  const std::string& name) const = 0;
    virtual void SaveFolder(const models::Folder& folder) = 0;
    virtual std::optional<models::Folder> FindFolder(const std::string& folder_id) const = 0;
    virtual std::vector<models::Folder>   FindFoldersByOwner(const std::string& owner_id) const = 0;

    virtual bool DeleteFolderCascade(const std::string& folder_id) = 0;

    // ── Файлы ─────────────────────────────────────────────────────────────────

    virtual bool FileNameExists(const std::string& folder_id,
                                const std::string& name) const = 0;
    virtual void SaveFile(const models::File& file) = 0;
    virtual std::optional<models::File> FindFileByName(const std::string& folder_id,
                                                       const std::string& name) const = 0;
    virtual std::optional<models::File> FindFileById(const std::string& file_id) const = 0;
    virtual bool DeleteFile(const std::string& file_id) = 0;
};

}  // namespace disk::folder_file_service
