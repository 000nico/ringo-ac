#pragma once

#define SystemBigPoolInformation 0x42

typedef struct _SYSTEM_BIGPOOL_ENTRY {
    union {
        PVOID  VirtualAddress;  // bit 0 used as flag flag, mask with & ~1 to get the real address
        ULONG_PTR NonPaged : 1; // 1 = NonPagedPool, 0 = PagedPool
    };
    SIZE_T SizeInBytes;
    union {
        UCHAR  Tag[4];
        ULONG  TagUlong;
    };
} SYSTEM_BIGPOOL_ENTRY, * PSYSTEM_BIGPOOL_ENTRY;

typedef struct _SYSTEM_BIGPOOL_INFORMATION {
    ULONG Count;
    SYSTEM_BIGPOOL_ENTRY AllocatedInfo[1]; // array variable, Count entries
} SYSTEM_BIGPOOL_INFORMATION, * PSYSTEM_BIGPOOL_INFORMATION;

void BigPool_Detect();