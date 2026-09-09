#pragma once
#include "../sdk/imports/imports.hpp"
#include "detection/vulnerable_drivers/vulnerable_driver_detections.hpp"
#include "detection/callbacks/image_notify/image_notify.hpp"
#include "detection/callbacks/create_thread_notify/create_thread_notify.hpp"
#include "detection/detections.hpp"

typedef struct RINGO_CONTEXT {
    struct {
        HANDLE ProtectedGamePid;
        UCHAR ProtectedGameProcessName[64];
        BOOLEAN isGameOpen;
    } Game;

    // todo: unificar cada callback en structuras para no tener repetidos
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
        BYOVD_DETECTION_REPORT ByovdDetectionReport;
        FLAGGED_DLL_REPORT DllDetectionReport;
        THREAD_DETECTION_REPORT ThreadDetectionReport;
    } Detections;

} RINGO_CONTEXT;

extern RINGO_CONTEXT ringo_context;

extern PDRIVER_OBJECT g_DriverObject;