#include "../../sdk/imports/imports.hpp"
#include "detections.hpp"

DETECTION_ENTRY CreateDriverDetectionEntry(PVOID suspiciousAddr, SIZE_T driverSize, DETECTION_METHOD method, UCHAR* modulePath) {
    DETECTION_ENTRY entry = {};

    entry.SuspiciousAddress = suspiciousAddr;
    entry.Size = driverSize;
    entry.Method = method;

    if (modulePath)
        RtlCopyMemory(entry.ModulePath, modulePath, sizeof(entry.ModulePath));

    return entry;
}