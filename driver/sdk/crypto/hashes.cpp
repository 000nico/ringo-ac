#include "../sdk.hpp"
#include <ntimage.h>
#include <bcrypt.h>

#define DRIVER_FILE_TAG 'hsrD'
#define SECTIONS_TAG 'ceSd'
#define MAX_DRIVER_FILE_SIZE 0x10000000
#define MAX_SECTIONS 96

static NTSTATUS read_file_from_disk(PUNICODE_STRING file_path, PVOID* out_buffer, ULONG* out_size) {
    OBJECT_ATTRIBUTES         obj_attr;
    IO_STATUS_BLOCK           io_status;
    HANDLE                    file_handle = NULL;
    NTSTATUS                  status = STATUS_SUCCESS;
    FILE_STANDARD_INFORMATION file_info = {};
    PVOID                     buffer = NULL;
    ULONG                     file_size = 0;
    LARGE_INTEGER             byte_offset = {};

    *out_buffer = NULL;
    *out_size = 0;

    InitializeObjectAttributes(
        &obj_attr,
        file_path,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
    );

    status = ZwCreateFile(
        &file_handle,
        GENERIC_READ | SYNCHRONIZE,
        &obj_attr,
        &io_status,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );
    if (!NT_SUCCESS(status))
        return status;

    status = ZwQueryInformationFile(
        file_handle,
        &io_status,
        &file_info,
        sizeof(file_info),
        FileStandardInformation
    );
    if (!NT_SUCCESS(status))
        goto cleanup;

    if (file_info.EndOfFile.QuadPart <= 0 || file_info.EndOfFile.QuadPart > MAX_DRIVER_FILE_SIZE) {
        status = STATUS_FILE_TOO_LARGE;
        goto cleanup;
    }

    file_size = (ULONG)file_info.EndOfFile.QuadPart;

    // Paged pool is enough, everything here runs at PASSIVE_LEVEL
    buffer = ExAllocatePool2(POOL_FLAG_PAGED, file_size, DRIVER_FILE_TAG);
    if (!buffer) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    status = ZwReadFile(
        file_handle,
        NULL,
        NULL,
        NULL,
        &io_status,
        buffer,
        file_size,
        &byte_offset,
        NULL
    );
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(buffer, DRIVER_FILE_TAG);
        goto cleanup;
    }

    if (io_status.Information != file_size) {
        ExFreePoolWithTag(buffer, DRIVER_FILE_TAG);
        status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    *out_buffer = buffer;
    *out_size = file_size;

cleanup:
    ZwClose(file_handle);
    return status;
}

static NTSTATUS hash_chunk(BCRYPT_HASH_HANDLE h_hash, PVOID data, ULONG size) {
    if (size == 0)
        return STATUS_SUCCESS;
    return BCryptHashData(h_hash, (PUCHAR)data, size, 0);
}

NTSTATUS compute_sha256(PVOID data, ULONG data_size, UCHAR output[HASH_SIZE_SHA256]) {
    BCRYPT_ALG_HANDLE  h_alg = NULL;
    BCRYPT_HASH_HANDLE h_hash = NULL;
    NTSTATUS           status;

    status = BCryptOpenAlgorithmProvider(&h_alg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if (!NT_SUCCESS(status))
        return status;

    status = BCryptCreateHash(h_alg, &h_hash, NULL, 0, NULL, 0, 0);
    if (!NT_SUCCESS(status))
        goto cleanup;

    status = hash_chunk(h_hash, data, data_size);
    if (!NT_SUCCESS(status))
        goto cleanup;

    status = BCryptFinishHash(h_hash, output, HASH_SIZE_SHA256, 0);

cleanup:
    if (h_hash) BCryptDestroyHash(h_hash);
    if (h_alg)  BCryptCloseAlgorithmProvider(h_alg, 0);
    return status;
}

static NTSTATUS compute_authentihash(PVOID file_data, ULONG file_size, UCHAR output[HASH_SIZE_SHA256]) {
    BCRYPT_ALG_HANDLE        h_alg = NULL;
    BCRYPT_HASH_HANDLE       h_hash = NULL;
    NTSTATUS                 status = STATUS_SUCCESS;
    PIMAGE_DOS_HEADER        dos_header = NULL;
    PIMAGE_NT_HEADERS64      nt_headers = NULL;
    PIMAGE_OPTIONAL_HEADER64 opt_header = NULL;
    PIMAGE_SECTION_HEADER    sorted_sections = NULL;
    ULONG                    checksum_offset = 0;
    ULONG                    sec_dir_offset = 0;
    ULONG                    after_checksum = 0;
    ULONG                    after_sec_dir = 0;
    ULONG                    header_size = 0;
    ULONG                    file_end_offset = 0;
    ULONG                    min_opt_header_size = 0;
    ULONGLONG                sum_of_bytes_hashed = 0;
    ULONGLONG                first_section_offset = 0;
    USHORT                   num_sections = 0;
    SIZE_T                   sections_bytes = 0;
    BOOLEAN                  has_sec_dir = FALSE;
    IMAGE_DATA_DIRECTORY     sec_dir = {};

    __try {
        if (file_size < sizeof(IMAGE_DOS_HEADER)) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        dos_header = (PIMAGE_DOS_HEADER)file_data;
        if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        if (dos_header->e_lfanew < (LONG)sizeof(IMAGE_DOS_HEADER) ||
            (ULONGLONG)dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > file_size) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        nt_headers = (PIMAGE_NT_HEADERS64)((PUCHAR)file_data + dos_header->e_lfanew);

        if (nt_headers->Signature != IMAGE_NT_SIGNATURE) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        // 64-bit images only
        if (nt_headers->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        opt_header = &nt_headers->OptionalHeader;

        checksum_offset = (ULONG)dos_header->e_lfanew
            + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader)
            + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER64, CheckSum);
        after_checksum = checksum_offset + sizeof(ULONG);

        header_size = opt_header->SizeOfHeaders;
        if (header_size > file_size || header_size < after_checksum) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        // The security directory entry may not exist in small optional headers
        min_opt_header_size = FIELD_OFFSET(IMAGE_OPTIONAL_HEADER64, DataDirectory)
            + (IMAGE_DIRECTORY_ENTRY_SECURITY + 1) * sizeof(IMAGE_DATA_DIRECTORY);

        has_sec_dir = opt_header->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_SECURITY &&
                    nt_headers->FileHeader.SizeOfOptionalHeader >= min_opt_header_size;

        if (has_sec_dir) {
            sec_dir_offset = (ULONG)dos_header->e_lfanew
                + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader)
                + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER64, DataDirectory)
                + IMAGE_DIRECTORY_ENTRY_SECURITY * sizeof(IMAGE_DATA_DIRECTORY);
            after_sec_dir = sec_dir_offset + (ULONG)sizeof(IMAGE_DATA_DIRECTORY);

            if (header_size < after_sec_dir) {
                status = STATUS_INVALID_IMAGE_FORMAT;
                goto cleanup;
            }
            sec_dir = opt_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        }

        // Validate section headers before touching them
        num_sections = nt_headers->FileHeader.NumberOfSections;
        if (num_sections == 0 || num_sections > MAX_SECTIONS) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        sections_bytes = (SIZE_T)num_sections * sizeof(IMAGE_SECTION_HEADER);
        first_section_offset = (ULONGLONG)dos_header->e_lfanew
            + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader)
            + nt_headers->FileHeader.SizeOfOptionalHeader;

        if (first_section_offset > file_size || sections_bytes > file_size - first_section_offset) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        // Validate certificate table location
        file_end_offset = file_size;
        if (sec_dir.VirtualAddress != 0 && sec_dir.Size != 0) {
            if (sec_dir.VirtualAddress > file_size || sec_dir.Size > file_size - sec_dir.VirtualAddress) {
                status = STATUS_INVALID_IMAGE_FORMAT;
                goto cleanup;
            }
            file_end_offset = sec_dir.VirtualAddress;
        }

        sorted_sections = (PIMAGE_SECTION_HEADER)ExAllocatePool2(
            POOL_FLAG_PAGED, sections_bytes, SECTIONS_TAG);
        if (!sorted_sections) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        RtlCopyMemory(sorted_sections, (PUCHAR)file_data + first_section_offset, sections_bytes);

        // Insertion sort by PointerToRawData
        for (LONG i = 1; i < (LONG)num_sections; i++) {
            IMAGE_SECTION_HEADER tmp = sorted_sections[i];
            LONG j = i - 1;
            while (j >= 0 && sorted_sections[j].PointerToRawData > tmp.PointerToRawData) {
                sorted_sections[j + 1] = sorted_sections[j];
                j--;
            }
            sorted_sections[j + 1] = tmp;
        }

        status = BCryptOpenAlgorithmProvider(&h_alg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
        if (!NT_SUCCESS(status))
            goto cleanup;

        status = BCryptCreateHash(h_alg, &h_hash, NULL, 0, NULL, 0, 0);
        if (!NT_SUCCESS(status))
            goto cleanup;

        // Headers: skip CheckSum and the security directory entry
        status = hash_chunk(h_hash, file_data, checksum_offset);
        if (!NT_SUCCESS(status))
            goto cleanup;

        if (has_sec_dir) {
            status = hash_chunk(h_hash, (PUCHAR)file_data + after_checksum, sec_dir_offset - after_checksum);
            if (!NT_SUCCESS(status))
                goto cleanup;

            status = hash_chunk(h_hash, (PUCHAR)file_data + after_sec_dir, header_size - after_sec_dir);
            if (!NT_SUCCESS(status))
                goto cleanup;
        }
        else {
            status = hash_chunk(h_hash, (PUCHAR)file_data + after_checksum, header_size - after_checksum);
            if (!NT_SUCCESS(status))
                goto cleanup;
        }

        sum_of_bytes_hashed = header_size;

        // Sections in file order
        for (USHORT i = 0; i < num_sections; i++) {
            ULONG raw_offset = sorted_sections[i].PointerToRawData;
            ULONG raw_size = sorted_sections[i].SizeOfRawData;

            if (raw_size == 0)
                continue;

            if ((ULONGLONG)raw_offset + raw_size > file_size) {
                status = STATUS_INVALID_IMAGE_FORMAT;
                goto cleanup;
            }

            status = hash_chunk(h_hash, (PUCHAR)file_data + raw_offset, raw_size);
            if (!NT_SUCCESS(status))
                goto cleanup;

            sum_of_bytes_hashed += raw_size;
        }

        // Extra data after the last section, up to the certificate table
        if (sum_of_bytes_hashed < file_end_offset) {
            status = hash_chunk(h_hash,
                (PUCHAR)file_data + (ULONG)sum_of_bytes_hashed,
                file_end_offset - (ULONG)sum_of_bytes_hashed);
            if (!NT_SUCCESS(status))
                goto cleanup;
        }

        status = BCryptFinishHash(h_hash, output, HASH_SIZE_SHA256, 0);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        status = STATUS_ACCESS_VIOLATION;
    }

cleanup:
    if (sorted_sections) ExFreePoolWithTag(sorted_sections, SECTIONS_TAG);
    if (h_hash)          BCryptDestroyHash(h_hash);
    if (h_alg)           BCryptCloseAlgorithmProvider(h_alg, 0);
    return status;
}

NTSTATUS compute_driver_hashes(PUNICODE_STRING full_image_name, PDRIVER_HASHES out_hashes) {
    PVOID    file_buffer = NULL;
    ULONG    file_size = 0;
    NTSTATUS status;

    if (!full_image_name || !out_hashes)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(out_hashes, sizeof(DRIVER_HASHES));

    status = read_file_from_disk(full_image_name, &file_buffer, &file_size);
    if (!NT_SUCCESS(status))
        return status;

    status = compute_sha256(file_buffer, file_size, out_hashes->sha256);
    out_hashes->sha256_valid = NT_SUCCESS(status);

    status = compute_authentihash(file_buffer, file_size, out_hashes->authentihash);
    out_hashes->authentihash_valid = NT_SUCCESS(status);

    ExFreePoolWithTag(file_buffer, DRIVER_FILE_TAG);

    return (out_hashes->sha256_valid || out_hashes->authentihash_valid)
        ? STATUS_SUCCESS
        : STATUS_UNSUCCESSFUL;
}