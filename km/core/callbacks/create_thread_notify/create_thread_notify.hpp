#pragma once
#include "sdk/sdk.hpp"

#define THREAD_QUERY_INFORMATION 0x0040

typedef struct _CREATED_THREAD {
    PVOID StartAddress;
    BOOLEAN is_created_remotely;
    HANDLE ThreadId;
    HANDLE CreatorId;
} CREATED_THREAD;

struct THREAD_DETECTION_REPORT {
    ULONG Count;
    CREATED_THREAD Entries[256];
};

VOID set_create_thread_callback();