#include "global.hpp"
#include "detection/detections.hpp"

DRIVER_DETECTION_REPORT g_DriverDetectionReport = {};

RINGO_CONTEXT ringo_context = { 0 };

PDRIVER_OBJECT g_DriverObject = NULL;