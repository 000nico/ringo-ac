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


#define SystemSecureBootInformation 145

typedef struct _SYSTEM_SECUREBOOT_INFORMATION {
    BOOLEAN SecureBootEnabled;
    BOOLEAN SecureBootCapable;
} SYSTEM_SECUREBOOT_INFORMATION, * PSYSTEM_SECUREBOOT_INFORMATION;

struct SECURE_BOOT_CHECK {
    SOURCE_RESULT nt_query_enabled;
    SOURCE_RESULT nt_query_capable;
    SOURCE_RESULT uefi_var;
    SOURCE_RESULT registry;
};

static_assert(sizeof(SECURE_BOOT_CHECK) == sizeof(SOURCE_RESULT) * 4, "SECURE_BOOT_CHECK layout changed");

SECURE_BOOT_CHECK is_secure_boot_enabled();