#pragma once

#include "service/file_system_service.hpp"

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/request/request_context.hpp>

namespace disk::handlers::files {

class DeleteFileHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-files-delete";

    DeleteFileHandler(const userver::components::ComponentConfig& config,
                      const userver::components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          fs_service_(context.FindComponent<folder_file_service::FileSystemService>()) {}

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

private:
    folder_file_service::FileSystemService& fs_service_;
};

void AppendDeleteFileHandler(userver::components::ComponentList& list);

}  // namespace disk::handlers::files
