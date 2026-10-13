file(GLOB_RECURSE DRIVER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

find_package(WDK REQUIRED)

wdk_add_driver(driver
    WINVER 0x0A00
    NTDDI_VERSION 0x0A000010
    ${DRIVER_SOURCES}
)

set_target_properties(driver PROPERTIES
    OUTPUT_NAME rngdrv
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(driver PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/cmake/wdk-shim"
)

target_link_libraries(driver
    WDK::CNG
    WDK::WDMSEC
)

ringo_set_output_dir(driver)
