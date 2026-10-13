#pragma once
#include "sdk/sdk.hpp"

enum BYOVD_DETECTION_METHOD {
	METHOD_IMAGE_CALLBACK,
	METHOD_DRIVER_WALKING,
	METHOD_RUNTIME_SCAN
};

struct BYOVD_DETECTION_ENTRY {
	WCHAR name_buffer[256];
	UNICODE_STRING full_image_name;
	PDEVICE_OBJECT device_object;
	PDRIVER_OBJECT driver_object;
	BYOVD_DETECTION_METHOD method;
};

struct BYOVD_DETECTION_REPORT {
	LONG count;
	BYOVD_DETECTION_ENTRY entries[256];
};

VOID report_byovd_detection(PUNICODE_STRING full_image_name, PDRIVER_OBJECT driver_object, PDEVICE_OBJECT device_object, BYOVD_DETECTION_METHOD method);
