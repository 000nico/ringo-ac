#pragma once
#include "sdk/sdk.hpp"

NTKERNELAPI NTSTATUS PsReferenceProcessFilePointer(_In_ PEPROCESS Process, _Outptr_ PFILE_OBJECT* FileObject);

bool get_caller_file_hash(PEPROCESS process, UCHAR sha256[HASH_SIZE_SHA256]);
bool get_caller_process_hash(PEPROCESS process, UCHAR sha256[HASH_SIZE_SHA256]);
bool compare_sha256(const UCHAR a[HASH_SIZE_SHA256], const UCHAR b[HASH_SIZE_SHA256]);
bool is_peprocess_banned(PEPROCESS peprocess);