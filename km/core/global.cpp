#include "global.hpp"
#include "detection/detections.hpp"

DRIVER_DETECTION_REPORT g_DriverDetectionReport = {};

// todo: change these and make someone similar to driver_detection_report
LOADED_MODULE g_LoadedDlls[256] = { 0 };
volatile LONG g_LoadedDllsCount = 0;

CREATED_THREAD g_CreatedThreads[256] = { 0 };
volatile LONG g_CreatedThreadsCount = 0;
RINGO_CONTEXT ringo_context = { 0 };

PDRIVER_OBJECT g_DriverObject = NULL;