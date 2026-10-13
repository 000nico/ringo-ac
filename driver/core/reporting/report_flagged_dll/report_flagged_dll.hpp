#pragma once
#include "sdk/sdk.hpp"

struct LOADED_MODULE {
    WCHAR          name_buffer[256];
    UNICODE_STRING full_image_name;
    BOOLEAN        is_signed;

    struct {
        struct {
            PVOID  image_base;
            SIZE_T image_size;
        } image;
    } mem;
};

struct FLAGGED_DLL_REPORT {
    LONG          count;
    LOADED_MODULE entries[256];
};

VOID report_flagged_dll(PUNICODE_STRING full_image_name, BOOLEAN is_signed, PVOID image_base, SIZE_T image_size);
