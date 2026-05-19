#pragma once

#include "i_file_system_repository.hpp"

#include <userver/storages/mongo/pool.hpp>

namespace disk::folder_file_service {

class MongoFileSystemRepository final : public IFileSystemRepository {
public:
    explicit MongoFileSystemRepository(userver::storages::mongo::PoolPtr pool);


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
    userver::storages::mongo::PoolPtr pool_;

    static models::Folder DocToFolder(const userver::formats::bson::Document& doc);
    static models::File   DocToFile(const userver::formats::bson::Document& doc);
};

}  // namespace disk::folder_file_service
