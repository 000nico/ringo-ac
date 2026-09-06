#pragma once
#include "../../../../sdk/imports/imports.hpp"

#define THREAD_QUERY_INFORMATION 0x0040

typedef struct _CREATED_THREAD {
    PVOID StartAddress;
    BOOLEAN isCreatedRemotely;
    HANDLE ThreadId;
    HANDLE CreatorId;
} CREATED_THREAD;

VOID SetCreateThreadCallback();