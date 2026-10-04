#pragma once
#include "sdk/sdk.hpp"

#define SystemModuleInformation 0xB

// nt structs
typedef struct _RTL_PROCESS_MODULE_INFORMATION {
    HANDLE Section;
    PVOID  MappedBase;
    PVOID  image_base;   // important
    ULONG  image_size;   // important
    ULONG  Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR  full_path_name[256];
} RTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
    ULONG number_of_modules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1]; // not the real size
} RTL_PROCESS_MODULES;

// ringo struct
struct LOADED_LEGIT_DRIVER_ENTRY {
    PVOID image_base;
    ULONG image_size;
    UCHAR full_path_name[256];
};

struct LOADED_LEGIT_DRIVERS {
    ULONG Count;
    LOADED_LEGIT_DRIVER_ENTRY Drivers[1];
};

// exposing fns
LOADED_LEGIT_DRIVERS* get_legit_driver_list();
VOID free_legit_driver_list(LOADED_LEGIT_DRIVERS* lld);
BOOLEAN is_address_in_known_module(PVOID address, LOADED_LEGIT_DRIVERS* lld);
BOOLEAN is_region_executable(PVOID address);