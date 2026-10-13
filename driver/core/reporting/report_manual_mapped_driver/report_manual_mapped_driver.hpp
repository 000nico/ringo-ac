#pragma once
#include "sdk/sdk.hpp"

enum DETECTION_METHOD {
    METHOD_BIG_POOL,
    METHOD_KERNEL_SPACE_WALKING
};

struct MANUAL_MAPPED_DRIVER_DETECTION_ENTRY {
    PVOID            suspicious_address;
    SIZE_T           size;
    DETECTION_METHOD method;           // which method detected it
    UCHAR            tag[4];           // pool tag if necessary
    UCHAR            module_path[256]; // path if it was resolved
};

struct MANUAL_MAPPED_DRIVER_DETECTION_REPORT {
    LONG                                 count;
    MANUAL_MAPPED_DRIVER_DETECTION_ENTRY entries[256];
};

VOID report_manual_mapped_driver(PVOID suspicious_address, SIZE_T driver_size, DETECTION_METHOD method, UCHAR* module_path);
