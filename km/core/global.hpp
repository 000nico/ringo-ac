// global.hpp — solo declaraciones
#pragma once
#include "detection/usermode_cheats/callbacks/image_notify/image_notify.hpp"
#include "detection/usermode_cheats/callbacks/create_thread_notify/create_thread_notify.hpp"

typedef struct RINGO_CONTEXT {
    HANDLE ProtectedGamePid;
    PUCHAR ProtectedGameProcessName;
    BOOLEAN isGameOpen;
    struct {
        PVOID ObCallbackHandle;
        BOOLEAN isLoadImageCallbackRegistered;
        PLOAD_IMAGE_NOTIFY_ROUTINE LoadImageCallbackPtr;
        BOOLEAN isCreateThreadCallbackRegistered;
        PCREATE_THREAD_NOTIFY_ROUTINE CreateThreadCallbackPtr;
        BOOLEAN isCreateProcessCallbackRegistered;
        PCREATE_PROCESS_NOTIFY_ROUTINE_EX CreateProcessNotifyRoutinePtr;
    } Callbacks;
} RINGO_CONTEXT;

extern LOADED_MODULE g_LoadedDlls[256];
extern volatile LONG g_LoadedDllsCount;

extern LOADED_MODULE g_FlaggedDlls[256];
extern volatile LONG g_FlaggedDllsCount;

extern CREATED_THREAD g_CreatedThreads[256];
extern volatile LONG g_CreatedThreadsCount;

extern RINGO_CONTEXT ringo_context;