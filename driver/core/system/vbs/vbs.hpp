#pragma once
#include "../system.hpp"

#ifdef _KERNEL_MODE
#include "sdk/sdk.hpp"
#else
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#endif


#define SystemIsolatedUserModeInformation 165

typedef struct _SYSTEM_ISOLATED_USER_MODE_INFORMATION {
    BOOLEAN SecureKernelRunning;
    BOOLEAN TrustletRunning;
    BOOLEAN IumEnabled;
    BOOLEAN IsWinlogonIUM;
    BOOLEAN EncryptionKeyAvailable;
    BOOLEAN Spare0[3];
    BOOLEAN TrustletsAllowed;
    BOOLEAN HvciEnabled;
    BOOLEAN HvciStrictMode;
    BOOLEAN Spare1[5];
    ULONGLONG Spare2;
} SYSTEM_ISOLATED_USER_MODE_INFORMATION;

struct VBS_CHECK {
    SOURCE_RESULT nt_query_secure_kernel;
    SOURCE_RESULT nt_query_hvci;
};

static_assert(sizeof(VBS_CHECK) == sizeof(SOURCE_RESULT) * 2, "VBS_CHECK layout changed");

VBS_CHECK is_vbs_enabled();