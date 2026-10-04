#pragma once
#include "sdk/sdk.hpp"

// todo: rename with manual map driver detections

enum DETECTION_METHOD {
    METHOD_BIG_POOL,
    METHOD_KERNEL_SPACE_WALKING
};

struct MANUAL_MAPPED_DRIVER_DETECTION_ENTRY {
    PVOID           SuspiciousAddress;
    SIZE_T          Size;
    DETECTION_METHOD Method;           // which metod detected
    UCHAR           Tag[4];            // pool tag if necessary
    UCHAR           ModulePath[256];   // path if it was resolved
};

struct MANUAL_MAPPED_DRIVER_DETECTION_REPORT {
    ULONG            Count;
    MANUAL_MAPPED_DRIVER_DETECTION_ENTRY  Entries[256];
};

MANUAL_MAPPED_DRIVER_DETECTION_ENTRY create_manual_mapped_driver_report_entry(PVOID suspiciousAddr, SIZE_T driverSize, DETECTION_METHOD method, UCHAR* modulePath);