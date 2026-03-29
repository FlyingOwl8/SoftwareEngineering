#pragma once

#include "service/user_service.hpp"

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/request/request_context.hpp>

namespace disk::handlers::auth {

class RegisterHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-auth-register";

    RegisterHandler(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          user_service_(context.FindComponent<user_service::UserService>()) {}

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

private:
    user_service::UserService& user_service_;
};

void AppendRegisterHandler(userver::components::ComponentList& list);

}  // namespace disk::handlers::auth
