cmake_minimum_required(VERSION 3.20)

project(cpp_webserver)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)

add_executable(server
    server_linux.cpp
    http_tcpServer_linux.cpp
    rest_pipeline.cpp
    server_config.cpp
    logging.cpp
)

target_link_libraries(server
    PRIVATE
    spdlog::spdlog
)