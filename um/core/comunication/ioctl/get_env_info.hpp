#pragma once

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
    InfoUnloadDriver
} ENVIRONMENT_INFO_CLASS, * PENVIRONMENT_INFO_CLASS;

BOOLEAN GetEnvironmentInformation(HANDLE hDriver, ENVIRONMENT_INFO_CLASS informationClass, PVOID buffer, ULONG size, DWORD* bytesReturned);