#pragma once
#include "../../../../sdk/imports/imports.hpp" 

struct LOADED_MODULE {  
    WCHAR NameBuffer[256];
    UNICODE_STRING FullImageName;
    BOOLEAN isSigned;

    struct {
        struct {
            PVOID ImageBase;
            SIZE_T ImageSize;
        } Image;
    } Mem;
};

struct FLAGGED_DLL_REPORT { 
    ULONG Count;
    LOADED_MODULE Entries[256];
};

NTSTATUS GetDriverObjectFromImageName(PUNICODE_STRING FullImageName, PDRIVER_OBJECT* OutDriverObject);
VOID SetLoadImageCallback();