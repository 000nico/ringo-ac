#pragma once
#include "../system.hpp"

#ifdef _KERNEL_MODE
#include "sdk/sdk.hpp"
#else
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#endif

#define SystemHypervisorInformation 159

typedef struct _SYSTEM_HYPERVISOR_INFORMATION {
    BOOLEAN HypervisorPresent;
    BOOLEAN HypervisorDebuggingEnabled;
    BOOLEAN NoWakeInterruptsPresent;
    BOOLEAN ExpiredTimerVector;
} SYSTEM_HYPERVISOR_INFORMATION, * PSYSTEM_HYPERVISOR_INFORMATION;;

struct HYPERVISOR_CHECK {
    SOURCE_RESULT nt_query;
    SOURCE_RESULT cpuid;
};

static_assert(sizeof(HYPERVISOR_CHECK) == sizeof(SOURCE_RESULT) * 2, "HYPERVISOR_CHECK layout changed");

HYPERVISOR_CHECK get_hypervisor_info();