#include "folder_consumer_handler.hpp"

#include <chrono>
#include <string>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/mongo/component.hpp>

namespace disk::folder_consumer {

FolderConsumerHandler::FolderConsumerHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      repository_(std::make_unique<folder_file_service::MongoFileSystemRepository>(
          context.FindComponent<userver::components::Mongo>("mongo-db").GetPool())),
      consumer_scope_(
          context.FindComponent<userver::kafka::ConsumerComponent>("kafka-consumer-folder")
              .GetConsumer())
{}

void FolderConsumerHandler::OnAllComponentsLoaded() {
    consumer_scope_.Start([this](userver::kafka::MessageBatchView batch) {
        ProcessBatch(batch);
    });
}

void FolderConsumerHandler::OnAllComponentsAreStopping() {
    consumer_scope_.Stop();
}

void FolderConsumerHandler::ProcessBatch(
    userver::kafka::MessageBatchView batch)
{
    for (const auto& msg : batch) {
        try {
            const auto payload = msg.GetPayload();
            const auto json    = userver::formats::json::FromString(std::string(payload));

            models::Folder folder;
            folder.id       = json["id"].As<std::string>();
            folder.name     = json["name"].As<std::string>();
            folder.owner_id = json["owner_id"].As<std::string>();

            const auto unix_sec = json["created_at_unix"].As<int64_t>();
            folder.created_at   = models::TimePoint{std::chrono::seconds(unix_sec)};

            repository_->SaveFolder(folder);

            LOG_INFO() << "FolderConsumer: saved folder id=" << folder.id
                       << " name=" << folder.name
                       << " owner=" << folder.owner_id;
        } catch (const std::exception& e) {
            LOG_ERROR() << "FolderConsumer: failed to process message: " << e.what();
        }
    }
    consumer_scope_.AsyncCommit();
}

void AppendFolderConsumerHandler(userver::components::ComponentList& list) {
    list.Append<FolderConsumerHandler>();
}

}  // namespace disk::folder_consumer
