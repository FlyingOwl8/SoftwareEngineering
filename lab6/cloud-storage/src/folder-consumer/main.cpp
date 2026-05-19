#include "folder_consumer_handler.hpp"

#include <userver/components/minimal_component_list.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/kafka/consumer_component.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalComponentList();

    component_list.Append<userver::clients::dns::Component>();
    component_list.Append<userver::components::DefaultSecdistProvider>();
    component_list.Append<userver::components::Secdist>();
    component_list.Append<userver::components::Mongo>("mongo-db");
    component_list.Append<userver::kafka::ConsumerComponent>("kafka-consumer-folder");

    disk::folder_consumer::AppendFolderConsumerHandler(component_list);

    return userver::utils::DaemonMain(argc, argv, component_list);
}
