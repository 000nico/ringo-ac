#pragma once
#include "../../../../sdk/imports/imports.hpp"

#define PROCESS_VM_READ        0x0010
#define PROCESS_VM_WRITE       0x0020
#define PROCESS_VM_OPERATION   0x0008

#define THREAD_DIRECT_IMPERSONATION 0x0200

PVOID ObRegisterCallbacksOpenProcess();