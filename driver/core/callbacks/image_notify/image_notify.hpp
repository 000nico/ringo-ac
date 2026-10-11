#pragma once
#include "sdk/sdk.hpp"

struct LOADED_MODULE {  
    WCHAR NameBuffer[256];
    UNICODE_STRING FullImageName;
    BOOLEAN isSigned;

    struct {
        struct {
            PVOID image_base;
            SIZE_T image_size;
        } Image;
    } Mem;
};

struct FLAGGED_DLL_REPORT { 
    ULONG Count;
    LOADED_MODULE Entries[256];
};

NTSTATUS get_driver_object_from_image_name(PUNICODE_STRING FullImageName, PDRIVER_OBJECT* OutDriverObject);
VOID set_load_image_callback();