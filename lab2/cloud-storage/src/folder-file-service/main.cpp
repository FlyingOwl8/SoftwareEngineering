#include "auth/jwt_utils.hpp"
#include "auth/jwt_auth_checker.hpp"
#include "auth/jwt_auth_factory.hpp"
#include "service/file_system_service.hpp"
#include "handlers/folders/create_folder_handler.hpp"
#include "handlers/folders/list_folders_handler.hpp"
#include "handlers/folders/delete_folder_handler.hpp"
#include "handlers/files/create_file_handler.hpp"
#include "handlers/files/get_file_handler.hpp"
#include "handlers/files/delete_file_handler.hpp"

#include "exceptions.hpp"

#include <cstdlib>
#include <memory>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
    const char* secret = std::getenv("JWT_SECRET");
    const char* exp    = std::getenv("JWT_EXPIRATION_HOURS");

    if (!secret) throw disk::exceptions::MissingEnvVarException("JWT_SECRET");
    if (!exp)    throw disk::exceptions::MissingEnvVarException("JWT_EXPIRATION_HOURS");

    disk::auth::InitJwt(secret, std::stoi(exp));

    auto component_list = userver::components::MinimalServerComponentList();

    component_list.Append<userver::components::TestsuiteSupport>();
    component_list.Append<userver::clients::dns::Component>();
    component_list.Append<userver::components::Mongo>("mongo-db");

    component_list.Append<disk::auth::JwtAuthComponent>();
    userver::server::handlers::auth::RegisterAuthCheckerFactory(
        disk::auth::JwtAuthCheckerFactory::kAuthType,
        std::make_unique<disk::auth::JwtAuthCheckerFactory>());

    disk::folder_file_service::AppendFileSystemService(component_list);

    disk::handlers::folders::AppendCreateFolderHandler(component_list);
    disk::handlers::folders::AppendListFoldersHandler(component_list);
    disk::handlers::folders::AppendDeleteFolderHandler(component_list);
    disk::handlers::files::AppendCreateFileHandler(component_list);
    disk::handlers::files::AppendGetFileHandler(component_list);
    disk::handlers::files::AppendDeleteFileHandler(component_list);

    return userver::utils::DaemonMain(argc, argv, component_list);
}
