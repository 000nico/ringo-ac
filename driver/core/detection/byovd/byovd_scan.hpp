#pragma once
#include <ntifs.h>

BOOLEAN is_driver_vulnerable(PUNICODE_STRING FullDriverName);
VOID detect_byovd_drivers_at_runtime();