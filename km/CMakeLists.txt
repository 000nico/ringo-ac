set(KM_SOURCES
    core/communication/dispatcher.cpp
    core/communication/device_setup.cpp
    core/communication/protection.cpp
    core/callbacks/create_process_notify/create_process_notify.cpp
    core/callbacks/create_thread_notify/create_thread_notify.cpp
    core/callbacks/image_notify/image_notify.cpp
    core/callbacks/ob_callback/ob_callback.cpp
    core/detection/manual_mapped_drivers/big_pool_track/big_pool_track.cpp
    core/detection/manual_mapped_drivers/driver_scan.cpp
    core/detection/manual_mapped_drivers/kernel_space_walking/kernel_space_walking.cpp
    core/detection/byovd/byovd_scan.cpp
    core/reporting/report_byovd/report_byovd.cpp
    core/reporting/report_manual_mapped_driver/report_manual_mapped_driver.cpp
    core/system/hvci/hvci.cpp
    core/system/hypervisor/hypervisor.cpp
    core/system/kernel_debugging/kernel_debugging.cpp
    core/system/secure_boot/secure_boot.cpp
    core/system/test_signing/test_signing.cpp
    core/system/tpm/tpm.cpp
    core/system/vbs/vbs.cpp
    core/global.cpp
    core/identity/hwid/hwid.cpp
    main.cpp
    sdk/crypto/hashes.cpp
    sdk/pe/pe.cpp
)

set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_EXE_LINKER_FLAGS "")
add_executable(km_driver ${KM_SOURCES})
set_target_properties(km_driver PROPERTIES
    OUTPUT_NAME rngdrv
    SUFFIX ".sys"
    RUNTIME_OUTPUT_DIRECTORY "${RINGO_BIN_DIR}"
    MSVC_RUNTIME_LIBRARY "MultiThreaded"
)

target_include_directories(km_driver PRIVATE
    "${RINGO_WDK_INCLUDE}/km"
    "${RINGO_WDK_INCLUDE}/km/crt"
    "${RINGO_WDK_INCLUDE}/shared"
    "${RINGO_SDK_INCLUDE}/shared"
    "${RINGO_SDK_INCLUDE}/um"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(km_driver PRIVATE
    _AMD64_
    AMD64
    _WIN64
    _WIN32_WINNT=0x0A00
    NTDDI_VERSION=0x0A000010
)

target_compile_options(km_driver PRIVATE
    /kernel
    /W3
    /EHs-c-
    /GR-
    /GS-
)

target_link_options(km_driver PRIVATE
    /driver
    /subsystem:native
    /entry:DriverEntry
    /nodefaultlib
    /integritycheck
)

target_link_directories(km_driver PRIVATE "${RINGO_WDK_KM_LIB}")
target_link_libraries(km_driver PRIVATE
    ntoskrnl.lib
    hal.lib
    bufferoverflowfastfailk.lib
    cng.lib
    wdmsec.lib
)
