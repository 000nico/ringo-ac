#pragma once
#include "../../../sdk/imports/imports.hpp"

#define SystemModuleInformation 0xB

// nt structs
typedef struct _RTL_PROCESS_MODULE_INFORMATION {
    HANDLE Section;
    PVOID  MappedBase;
    PVOID  ImageBase;   // important
    ULONG  ImageSize;   // important
    ULONG  Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR  FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1]; // not the real size
} RTL_PROCESS_MODULES;

// ringo struct
struct LOADED_LEGIT_DRIVER_ENTRY {
    PVOID ImageBase;
    ULONG ImageSize;
    UCHAR FullPathName[256];
};

struct LOADED_LEGIT_DRIVERS {
    ULONG Count;
    LOADED_LEGIT_DRIVER_ENTRY* Drivers;
};

// exposing fns
LOADED_LEGIT_DRIVERS GetLegitDriverList();
BOOLEAN isAddressInKnownModule(PVOID address, LOADED_LEGIT_DRIVERS* lld);