#pragma once
#include "sdk/sdk.hpp"

#define PROCESS_VM_READ        0x0010
#define PROCESS_VM_WRITE       0x0020
#define PROCESS_VM_OPERATION   0x0008

#define THREAD_DIRECT_IMPERSONATION 0x0200

PVOID ob_register_callbacks_open_process();