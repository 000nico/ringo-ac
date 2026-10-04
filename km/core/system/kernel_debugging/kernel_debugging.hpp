#pragma once
#include "../system.hpp"
#define SystemKernelDebuggerInformation 35

typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION {
    BOOLEAN KernelDebuggerEnabled;
    BOOLEAN KernelDebuggerNotPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION;

struct KERNEL_DEBUGGING_CHECK {
    SOURCE_RESULT nt_query_enabled;
    SOURCE_RESULT nt_query_not_present;
};

static_assert(sizeof(KERNEL_DEBUGGING_CHECK) == sizeof(SOURCE_RESULT) * 2, "KERNEL_DEBUGGING_CHECK layout changed");

KERNEL_DEBUGGING_CHECK is_kernel_debugger_present();