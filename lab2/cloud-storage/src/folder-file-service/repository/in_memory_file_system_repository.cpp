#include "in_memory_file_system_repository.hpp"

#include <mutex>

namespace disk::folder_file_service {

// ── Папки ─────────────────────────────────────────────────────────────────────

bool InMemoryFileSystemRepository::FolderNameExists(
    const std::string& owner_id,
    const std::string& name) const
{
    std::shared_lock lock(mutex_);
    for (const auto& [id, f] : folders_) {
        if (f.owner_id == owner_id && f.name == name) return true;
    }
    return false;
}

void InMemoryFileSystemRepository::SaveFolder(const models::Folder& folder) {
    std::unique_lock lock(mutex_);
    folders_[folder.id] = folder;
}

std::optional<models::Folder> InMemoryFileSystemRepository::FindFolder(
    const std::string& folder_id) const
{
    std::shared_lock lock(mutex_);
    auto it = folders_.find(folder_id);
    if (it == folders_.end()) return std::nullopt;
    return it->second;
}

std::vector<models::Folder> InMemoryFileSystemRepository::FindFoldersByOwner(
    const std::string& owner_id) const
{
    std::shared_lock lock(mutex_);
    std::vector<models::Folder> result;
    for (const auto& [id, f] : folders_) {
        if (f.owner_id == owner_id) result.push_back(f);
    }
    return result;
}

bool InMemoryFileSystemRepository::DeleteFolderCascade(const std::string& folder_id) {
    std::unique_lock lock(mutex_);
    if (!folders_.count(folder_id)) return false;

    for (auto it = files_.begin(); it != files_.end(); ) {
        if (it->second.folder_id == folder_id) it = files_.erase(it);
        else ++it;
    }
    folders_.erase(folder_id);
    return true;
}

// ── Файлы ─────────────────────────────────────────────────────────────────────

bool InMemoryFileSystemRepository::FileNameExists(
    const std::string& folder_id,
    const std::string& name) const
{
    std::shared_lock lock(mutex_);
    for (const auto& [id, f] : files_) {
        if (f.folder_id == folder_id && f.name == name) return true;
    }
    return false;
}

void InMemoryFileSystemRepository::SaveFile(const models::File& file) {
    std::unique_lock lock(mutex_);
    files_[file.id] = file;
}

std::optional<models::File> InMemoryFileSystemRepository::FindFileByName(
    const std::string& folder_id,
    const std::string& name) const
{
    std::shared_lock lock(mutex_);
    for (const auto& [id, f] : files_) {
        if (f.folder_id == folder_id && f.name == name) return f;
    }
    return std::nullopt;
}

std::optional<models::File> InMemoryFileSystemRepository::FindFileById(
    const std::string& file_id) const
{
    std::shared_lock lock(mutex_);
    auto it = files_.find(file_id);
    if (it == files_.end()) return std::nullopt;
    return it->second;
}

bool InMemoryFileSystemRepository::DeleteFile(const std::string& file_id) {
    std::unique_lock lock(mutex_);
    return files_.erase(file_id) > 0;
}

}  // namespace disk::folder_file_service
