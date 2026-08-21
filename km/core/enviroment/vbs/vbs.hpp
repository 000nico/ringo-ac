#pragma once
#include <ntddk.h>

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
    BOOLEAN isVBSEnabled;
    BOOLEAN isHVCIEnabled;
};

VBS_CHECK isVBSEnabled();