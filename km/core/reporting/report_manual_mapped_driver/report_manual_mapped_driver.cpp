#include "sdk/sdk.hpp"
#include "report_manual_mapped_driver.hpp"

MANUAL_MAPPED_DRIVER_DETECTION_ENTRY create_manual_mapped_driver_report_entry(PVOID suspiciousAddr, SIZE_T driverSize, DETECTION_METHOD method, UCHAR* modulePath) {
    MANUAL_MAPPED_DRIVER_DETECTION_ENTRY entry = {};

    entry.SuspiciousAddress = suspiciousAddr;
    entry.Size = driverSize;
    entry.Method = method;

    if (modulePath)
        RtlCopyMemory(entry.ModulePath, modulePath, sizeof(entry.ModulePath));

    return entry;
}