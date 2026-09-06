#pragma once
#include "../../../../sdk/imports/imports.hpp"

#define THREAD_QUERY_INFORMATION 0x0040

typedef struct _CREATED_THREAD {
    PVOID StartAddress;
    BOOLEAN isCreatedRemotely;
    HANDLE ThreadId;
    HANDLE CreatorId;
} CREATED_THREAD;

struct THREAD_DETECTION_REPORT {
    ULONG Count;
    CREATED_THREAD Entries[256];
};

VOID SetCreateThreadCallback();