#pragma once

#ifdef _KERNEL_MODE
#include <ntddk.h>
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
    BOOLEAN hypervisorPresent;
    BOOLEAN manipulatedInformation;
};

HYPERVISOR_CHECK getHypervisorInfo();