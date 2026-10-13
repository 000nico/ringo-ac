#pragma once
#include "sdk/sdk.hpp"
#include "core/reporting/report_flagged_dll/report_flagged_dll.hpp"

NTSTATUS get_driver_object_from_image_name(PUNICODE_STRING full_image_name, PDRIVER_OBJECT* out_driver_object);
VOID set_load_image_callback();
