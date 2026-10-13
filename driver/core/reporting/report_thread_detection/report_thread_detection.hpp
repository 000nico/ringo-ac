#pragma once
#include "sdk/sdk.hpp"

struct CREATED_THREAD {
    PVOID   start_address;
    BOOLEAN is_created_remotely;
    HANDLE  thread_id;
    HANDLE  creator_id;
};

struct THREAD_DETECTION_REPORT {
    LONG           count;
    CREATED_THREAD entries[256];
};

VOID report_thread_detection(PVOID start_address, BOOLEAN is_created_remotely, HANDLE thread_id, HANDLE creator_id);
