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

set(SERVER_SOURCES
    core/analysis/engine.cpp
    core/analysis/rules/byovd/byovd.cpp
    core/analysis/rules/integrity/integrity.cpp
    core/analysis/rules/manual_mapped_drivers/manual_mapped_drivers.cpp
    core/analysis/rules/system/hvci/hvci.cpp
    core/analysis/rules/system/hypervisor/hypervisor.cpp
    core/analysis/rules/system/kernel_debugger/kernel_debugger.cpp
    core/analysis/rules/system/secure_boot/secure_boot.cpp
    core/analysis/rules/system/test_signing/test_signing.cpp
    core/analysis/rules/system/tpm/tpm.cpp
    core/analysis/rules/system/vbs/vbs.cpp
    core/api/routes/telemetry.cpp
    core/api/routes/verdict.cpp
    core/api/server.cpp
    core/db/db.cpp
    core/db/reports.cpp
    core/db/verdict.cpp
    main.cpp
)

add_executable(ringo-server ${SERVER_SOURCES})

target_include_directories(ringo-server PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
)

target_link_libraries(ringo-server
    PRIVATE
    SQLiteCpp
    nlohmann_json::nlohmann_json
    ws2_32
    crypt32
)

ringo_set_output_dir(ringo-server)
