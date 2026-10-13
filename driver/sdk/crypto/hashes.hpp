#pragma once
#include <ntifs.h>
#include <bcrypt.h>

#pragma comment(lib, "cng.lib")

#define HASH_SIZE_SHA256 32

typedef struct _DRIVER_HASHES {
    UCHAR   sha256[HASH_SIZE_SHA256];
    UCHAR   authentihash[HASH_SIZE_SHA256];
    BOOLEAN sha256_valid;
    BOOLEAN authentihash_valid;
} DRIVER_HASHES, * PDRIVER_HASHES;

NTSTATUS compute_driver_hashes(PUNICODE_STRING full_image_name, PDRIVER_HASHES out_hashes);
static NTSTATUS read_file_from_disk(PUNICODE_STRING file_path, PVOID* out_buffer, ULONG* out_size);
NTSTATUS compute_sha256(PVOID data, ULONG data_size, UCHAR output[HASH_SIZE_SHA256]);