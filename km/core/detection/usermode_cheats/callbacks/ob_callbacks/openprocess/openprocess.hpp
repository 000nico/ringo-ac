#pragma once
#include<ntifs.h>

#define PROCESS_VM_READ        0x0010
#define PROCESS_VM_WRITE       0x0020
#define PROCESS_VM_OPERATION   0x0008


PVOID ObRegisterCallbacksOpenProcess();