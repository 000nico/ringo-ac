include(FetchContent)

# Las dependencias traidas por FetchContent declaran versiones minimas viejas
# que CMake 4.x rechaza; permitimos esas politicas.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

# v0.60.0 es la misma version que el httplib.h vendorizado en server/third_party.
#
# Desactivamos todas las dependencias opcionales. Si no, el find_package() de
# httplib encuentra las libs de MSYS2 que estan en el PATH del sistema
# (C:/msys64/ucrt64) y arrastra C:/msys64/ucrt64/include a la linea de comandos
# de MSVC, que entonces incluye los headers de MinGW (winnt.h, corecrt.h...).
set(HTTPLIB_INSTALL              OFF CACHE BOOL "" FORCE)
set(HTTPLIB_TEST                 OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE    OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE  OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.60.0
)

FetchContent_MakeAvailable(json httplib)
file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(client ${CLIENT_SOURCES})

set_target_properties(client PROPERTIES
    OUTPUT_NAME ringo_client
    SUFFIX ".exe"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

target_include_directories(client PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/shared"
    "${CMAKE_SOURCE_DIR}/driver"
)

target_compile_definitions(client PRIVATE
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0A00
    _HAS_EXCEPTIONS=0
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    _CONSOLE
)

target_compile_options(client PRIVATE
    /W3
    /EHsc
    /GR
)

target_link_options(client PRIVATE
    /subsystem:console
)

target_link_libraries(client PRIVATE
    advapi32
    nlohmann_json::nlohmann_json
    httplib::httplib
)

ringo_set_output_dir(client)
