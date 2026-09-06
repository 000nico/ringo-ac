#pragma once
#include "../sdk/imports/imports.hpp"
#include "detection/callbacks/image_notify/image_notify.hpp"
#include "detection/callbacks/create_thread_notify/create_thread_notify.hpp"
#include "detection/detections.hpp"

typedef struct RINGO_CONTEXT {
    HANDLE ProtectedGamePid;
    UCHAR ProtectedGameProcessName[64];
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

    struct {
        DRIVER_DETECTION_REPORT DriverDetectionReport;
        FLAGGED_DLL_REPORT DllDetectionReport;
    } Detections;

} RINGO_CONTEXT;

extern LOADED_MODULE g_LoadedDlls[256];
extern volatile LONG g_LoadedDllsCount;

extern CREATED_THREAD g_CreatedThreads[256];
extern volatile LONG g_CreatedThreadsCount;

extern RINGO_CONTEXT ringo_context;

extern PDRIVER_OBJECT g_DriverObject;