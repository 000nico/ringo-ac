#pragma once
#include "../../sdk/imports/imports.hpp"

// todo: rename with manual map driver detections

enum DETECTION_METHOD {
    METHOD_BIG_POOL,
    METHOD_KERNEL_SPACE_WALKING
};

struct DETECTION_ENTRY {
    PVOID           SuspiciousAddress;
    SIZE_T          Size;
    DETECTION_METHOD Method;           // which metod detected
    UCHAR           Tag[4];            // pool tag if necessary
    UCHAR           ModulePath[256];   // path if it was resolved
};

struct DRIVER_DETECTION_REPORT {
    ULONG            Count;
    DETECTION_ENTRY  Entries[256];
};

DETECTION_ENTRY CreateDriverDetectionEntry(PVOID suspiciousAddr, SIZE_T driverSize, DETECTION_METHOD method, UCHAR* modulePath);