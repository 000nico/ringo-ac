set(UM_SOURCES
    core/communication/get_env_info.cpp
    core/communication/setters.cpp
    core/driver.cpp
    main.cpp
)

add_executable(um_app ${UM_SOURCES})
set_target_properties(um_app PROPERTIES
    OUTPUT_NAME ringo_client
    SUFFIX ".exe"
    RUNTIME_OUTPUT_DIRECTORY "${RINGO_BIN_DIR}"
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

target_include_directories(um_app PRIVATE
    "${RINGO_SDK_INCLUDE}/shared"
    "${RINGO_SDK_INCLUDE}/um"
    "${RINGO_SDK_INCLUDE}/ucrt"
    "${RINGO_CRT_INCLUDE}"
    "${CMAKE_SOURCE_DIR}/km"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(um_app PRIVATE
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0A00
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    _CONSOLE
)

target_compile_options(um_app PRIVATE
    /W3
    /EHsc
    /GR
    /D_HAS_EXCEPTIONS=0
)

target_link_options(um_app PRIVATE
    /subsystem:console
)

target_link_directories(um_app PRIVATE
    "${RINGO_SDK_UM_LIB}"
    "${RINGO_SDK_UCRT_LIB}"
    "${RINGO_CRT_LIB}"
)
