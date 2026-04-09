cmake_minimum_required(VERSION 3.12...3.31)
project(cloud-storage CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(USERVER_FEATURE_POSTGRESQL ON CACHE BOOL "" FORCE)
set(USERVER_FEATURE_MONGODB    ON CACHE BOOL "" FORCE)

find_package(userver COMPONENTS core postgresql mongodb QUIET)
if(NOT userver_FOUND)
    include(DownloadUserver)
    download_userver(
        TRY_DIR third_party/userver
        GIT_TAG 1e560caf6787b61b389032b7bec68f2d404f80b5
    )
endif()

userver_setup_environment()

# ─── jwt-cpp ──────────────────────────────────────────────────────────────────

include(FetchContent)
FetchContent_Declare(
    jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG        v0.7.0
)
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(jwt-cpp)

# ─── Общий код (JWT) ──────────────────────────────────────────────────────────

set(SHARED_SOURCES
    src/shared/auth/jwt_utils.cpp
    src/shared/auth/jwt_auth_checker.cpp
    src/shared/auth/jwt_auth_factory.cpp
)

# ─── user-service ─────────────────────────────────────────────────────────────

add_executable(user-service
    src/user-service/main.cpp
    src/user-service/repository/in_memory_user_repository.cpp
    src/user-service/repository/postgres_user_repository.cpp
    src/user-service/service/user_service.cpp
    src/user-service/handlers/auth/register_handler.cpp
    src/user-service/handlers/auth/login_handler.cpp
    src/user-service/handlers/users/get_by_login_handler.cpp
    src/user-service/handlers/users/search_handler.cpp
    ${SHARED_SOURCES}
)
target_link_libraries(user-service userver::core userver::postgresql jwt-cpp::jwt-cpp)
target_include_directories(user-service PRIVATE
    src/shared
    src/user-service
)

# ─── folder-file-service ──────────────────────────────────────────────────────

add_executable(folder-file-service
    src/folder-file-service/main.cpp
    src/folder-file-service/repository/in_memory_file_system_repository.cpp
    src/folder-file-service/repository/mongo_file_system_repository.cpp
    src/folder-file-service/service/file_system_service.cpp
    src/folder-file-service/handlers/folders/create_folder_handler.cpp
    src/folder-file-service/handlers/folders/list_folders_handler.cpp
    src/folder-file-service/handlers/folders/delete_folder_handler.cpp
    src/folder-file-service/handlers/files/create_file_handler.cpp
    src/folder-file-service/handlers/files/get_file_handler.cpp
    src/folder-file-service/handlers/files/delete_file_handler.cpp
    ${SHARED_SOURCES}
)
target_link_libraries(folder-file-service userver::core userver::mongo jwt-cpp::jwt-cpp)
target_include_directories(folder-file-service PRIVATE
    src/shared
    src/folder-file-service
)
