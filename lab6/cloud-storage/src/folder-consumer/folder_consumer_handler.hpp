#pragma once

#include "repository/mongo_file_system_repository.hpp"

#include <memory>

#include <userver/components/component_base.hpp>
#include <userver/components/component_list.hpp>
#include <userver/kafka/consumer_component.hpp>
#include <userver/kafka/consumer_scope.hpp>
#include <userver/kafka/message.hpp>
#include <userver/storages/mongo/component.hpp>

namespace disk::folder_consumer {

class FolderConsumerHandler final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "folder-consumer-handler";

    FolderConsumerHandler(const userver::components::ComponentConfig& config,
                          const userver::components::ComponentContext& context);

    void OnAllComponentsLoaded() override;
    void OnAllComponentsAreStopping() override;

private:
    void ProcessBatch(userver::kafka::MessageBatchView batch);

    std::unique_ptr<folder_file_service::MongoFileSystemRepository> repository_;
    userver::kafka::ConsumerScope consumer_scope_;
};

void AppendFolderConsumerHandler(userver::components::ComponentList& list);

}  // namespace disk::folder_consumer
