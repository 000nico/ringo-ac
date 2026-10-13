#pragma once
#include <ntifs.h>

BOOLEAN is_driver_vulnerable(PUNICODE_STRING full_driver_name);
VOID detect_byovd_drivers_at_runtime();