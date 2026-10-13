set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Las dependencias traidas por FetchContent declaran versiones minimas viejas
# que CMake 4.x rechaza; permitimos esas politicas.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_Declare(
    sqlitecpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        3.4.0
    GIT_SUBMODULES sqlite3
    GIT_SUBMODULES_RECURSE TRUE
)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

FetchContent_MakeAvailable(sqlitecpp json)
file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
list(FILTER SERVER_SOURCES EXCLUDE REGEX "/third_party/")

add_executable(ringo-server ${SERVER_SOURCES})

target_include_directories(ringo-server PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
    "${CMAKE_SOURCE_DIR}/shared"
    "${CMAKE_SOURCE_DIR}/driver"
)

target_link_libraries(ringo-server
    PRIVATE
    SQLiteCpp
    nlohmann_json::nlohmann_json
    ws2_32
    crypt32
)

ringo_set_output_dir(ringo-server)
