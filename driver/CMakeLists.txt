set(DRIVER_SOURCES
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
    core/detection/integrity/hash_sections/hash_sections.cpp
    core/reporting/report_byovd/report_byovd.cpp
    core/reporting/report_manual_mapped_driver/report_manual_mapped_driver.cpp
    core/reporting/report_hash_diff/report_hash_diff.cpp
    core/reporting/report_flagged_dll/report_flagged_dll.cpp
    core/reporting/report_thread_detection/report_thread_detection.cpp
    core/system/hvci/hvci.cpp
    core/system/hypervisor/hypervisor.cpp
    core/system/kernel_debugging/kernel_debugging.cpp
    core/system/secure_boot/secure_boot.cpp
    core/system/test_signing/test_signing.cpp
    core/system/tpm/tpm.cpp
    core/system/vbs/vbs.cpp
    core/heartbeat/heartbeat.cpp
    core/global.cpp
    core/identity/hwid/hwid.cpp
    core/scheduler/scheduler.cpp
    core/scheduler/tasks.cpp
    main.cpp
    sdk/crypto/hashes.cpp
    sdk/pe/pe.cpp
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
