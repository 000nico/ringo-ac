cmake_minimum_required(VERSION 3.20)
project(ringo-server)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
    sqlitecpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        3.4.0
)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

FetchContent_MakeAvailable(sqlitecpp json)

add_executable(ringo-server src/main.cpp)

target_link_libraries(ringo-server
    PRIVATE
    SQLiteCpp
    nlohmann_json::nlohmann_json
)

target_include_directories(ringo-server PRIVATE third_party)