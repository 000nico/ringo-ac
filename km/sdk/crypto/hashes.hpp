#pragma once
#include <ntifs.h>
#include <bcrypt.h>

#pragma comment(lib, "cng.lib")

#define HASH_SIZE_SHA256 32

typedef struct _DRIVER_HASHES {
    UCHAR   SHA256[HASH_SIZE_SHA256];
    UCHAR   Authentihash[HASH_SIZE_SHA256];
    BOOLEAN SHA256Valid;
    BOOLEAN AuthentihashValid;
} DRIVER_HASHES, * PDRIVER_HASHES;

NTSTATUS compute_driver_hashes(PUNICODE_STRING FullImageName, PDRIVER_HASHES outHashes);
static NTSTATUS read_file_from_disk(PUNICODE_STRING filePath, PVOID* outBuffer, ULONG* outSize);
NTSTATUS compute_sha256(PVOID data, ULONG dataSize, UCHAR output[HASH_SIZE_SHA256]);