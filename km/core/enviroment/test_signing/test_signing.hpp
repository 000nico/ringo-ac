#pragma once

#ifdef _KERNEL_MODE
#include <ntddk.h>
#else
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#endif


#define SystemCodeIntegrityInformation 103
#define CODEINTEGRITY_OPTION_ENABLED     0x01
#define CODEINTEGRITY_OPTION_TESTSIGN    0x02

typedef struct _SYSTEM_CODEINTEGRITY_INFORMATION {
    ULONG Length;
    ULONG CodeIntegrityOptions;
} SYSTEM_CODEINTEGRITY_INFORMATION, * PSYSTEM_CODEINTEGRITY_INFORMATION;

struct TEST_SIGNING_CHECK {
    BOOLEAN isEnabled;
};

TEST_SIGNING_CHECK isTestSigningEnabled();