set(CLIENT_SOURCES
    core/communication/get_env_info.cpp
    core/communication/setters.cpp
    core/driver.cpp
    main.cpp
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
)

ringo_set_output_dir(client)
