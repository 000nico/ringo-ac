#pragma once
#define SystemKernelDebuggerInformation 35

typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION {
    BOOLEAN KernelDebuggerEnabled;
    BOOLEAN KernelDebuggerNotPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION;

struct KERNEL_DEBUGGING_CHECK {
	BOOLEAN isEnabled;
};

KERNEL_DEBUGGING_CHECK isKernelDebuggerPresent();