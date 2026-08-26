#pragma once

#ifdef _KERNEL_MODE
#include <ntddk.h>
#else
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#endif


#define SystemSecureBootInformation 145

typedef struct _SYSTEM_SECUREBOOT_INFORMATION {
    BOOLEAN SecureBootEnabled;
    BOOLEAN SecureBootCapable;
} SYSTEM_SECUREBOOT_INFORMATION, * PSYSTEM_SECUREBOOT_INFORMATION;

struct SECURE_BOOT_CHECK {
    BOOLEAN isSupported;
    BOOLEAN isEnabled;
    BOOLEAN consistentData; // check if spoofed
};

SECURE_BOOT_CHECK isSecureBootEnabled();