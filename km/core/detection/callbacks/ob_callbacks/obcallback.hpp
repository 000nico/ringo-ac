#pragma once
#include <ntifs.h>

#define PROCESS_VM_READ        0x0010
#define PROCESS_VM_WRITE       0x0020
#define PROCESS_VM_OPERATION   0x0008

#define THREAD_TERMINATE            0x0001
#define THREAD_SUSPEND_RESUME       0x0002
#define THREAD_GET_CONTEXT          0x0008
#define THREAD_SET_CONTEXT          0x0010
#define THREAD_SET_INFORMATION      0x0020
#define THREAD_DIRECT_IMPERSONATION 0x0200

PVOID ObRegisterCallbacksOpenProcess();