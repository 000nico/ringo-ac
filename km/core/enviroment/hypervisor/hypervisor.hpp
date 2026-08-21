#pragma once
#include <ntddk.h>

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