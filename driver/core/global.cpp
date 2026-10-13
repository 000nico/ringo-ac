#include "global.hpp"
#include "reporting/report_manual_mapped_driver/report_manual_mapped_driver.hpp"

RINGO_CONTEXT g_ringo_context = { 0 };

PDRIVER_OBJECT g_DriverObject = NULL;
KEVENT g_stop_event = { 0 };