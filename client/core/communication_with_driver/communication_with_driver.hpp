#pragma once

#include <windows.h>
#include <ntstatus.h>

typedef enum _ENVIRONMENT_INFO_CLASS {
    InfoHwid = 1,
    InfoGamePid,
    InfoGameStatus,
    InfoHvci,
    InfoHypervisorStatus,
    InfoKernelDebuggingStatus,
    InfoSecureBoot,
    InfoTestSigningMode,
    InfoVbs,
    InfoTpm,
    InfoCreateProcessCallbackSet,
    InfoCreateThreadCallbackSet,
    InfoImageCallbackSet,
    InfoObCallbackSet,
    InfoSetGameName,
    InfoHeartbeat,
    InfoUnloadDriver
} ENVIRONMENT_INFO_CLASS, * PENVIRONMENT_INFO_CLASS;

BOOLEAN get_environment_information(HANDLE hDriver, ENVIRONMENT_INFO_CLASS informationClass, PVOID buffer, ULONG size, DWORD* bytesReturned);

NTSTATUS set_game_process_name(HANDLE hDriver, const char* name);
