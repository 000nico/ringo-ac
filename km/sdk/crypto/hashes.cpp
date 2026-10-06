#include "../sdk.hpp"
#include <ntimage.h>
#include <bcrypt.h>

#define DRIVER_FILE_TAG 'hsrD'
#define SECTIONS_TAG 'ceSd'
#define MAX_DRIVER_FILE_SIZE 0x10000000
#define MAX_SECTIONS 96

static NTSTATUS read_file_from_disk(PUNICODE_STRING filePath, PVOID* outBuffer, ULONG* outSize) {
    OBJECT_ATTRIBUTES         objAttr;
    IO_STATUS_BLOCK           ioStatus;
    HANDLE                    fileHandle = NULL;
    NTSTATUS                  status = STATUS_SUCCESS;
    FILE_STANDARD_INFORMATION fileInfo = {};
    PVOID                     buffer = NULL;
    ULONG                     fileSize = 0;
    LARGE_INTEGER             byteOffset = {};

    *outBuffer = NULL;
    *outSize = 0;

    InitializeObjectAttributes(
        &objAttr,
        filePath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
    );

    status = ZwCreateFile(
        &fileHandle,
        GENERIC_READ | SYNCHRONIZE,
        &objAttr,
        &ioStatus,
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
        fileHandle,
        &ioStatus,
        &fileInfo,
        sizeof(fileInfo),
        FileStandardInformation
    );
    if (!NT_SUCCESS(status))
        goto cleanup;

    if (fileInfo.EndOfFile.QuadPart <= 0 || fileInfo.EndOfFile.QuadPart > MAX_DRIVER_FILE_SIZE) {
        status = STATUS_FILE_TOO_LARGE;
        goto cleanup;
    }

    fileSize = (ULONG)fileInfo.EndOfFile.QuadPart;

    // Paged pool is enough, everything here runs at PASSIVE_LEVEL
    buffer = ExAllocatePool2(POOL_FLAG_PAGED, fileSize, DRIVER_FILE_TAG);
    if (!buffer) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    status = ZwReadFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &ioStatus,
        buffer,
        fileSize,
        &byteOffset,
        NULL
    );
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(buffer, DRIVER_FILE_TAG);
        goto cleanup;
    }

    if (ioStatus.Information != fileSize) {
        ExFreePoolWithTag(buffer, DRIVER_FILE_TAG);
        status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    *outBuffer = buffer;
    *outSize = fileSize;

cleanup:
    ZwClose(fileHandle);
    return status;
}

static NTSTATUS hash_chunk(BCRYPT_HASH_HANDLE hHash, PVOID data, ULONG size) {
    if (size == 0)
        return STATUS_SUCCESS;
    return BCryptHashData(hHash, (PUCHAR)data, size, 0);
}

NTSTATUS compute_sha256(PVOID data, ULONG dataSize, UCHAR output[HASH_SIZE_SHA256]) {
    BCRYPT_ALG_HANDLE  hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    NTSTATUS           status;

    status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if (!NT_SUCCESS(status))
        return status;

    status = BCryptCreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0);
    if (!NT_SUCCESS(status))
        goto cleanup;

    status = hash_chunk(hHash, data, dataSize);
    if (!NT_SUCCESS(status))
        goto cleanup;

    status = BCryptFinishHash(hHash, output, HASH_SIZE_SHA256, 0);

cleanup:
    if (hHash) BCryptDestroyHash(hHash);
    if (hAlg)  BCryptCloseAlgorithmProvider(hAlg, 0);
    return status;
}

static NTSTATUS compute_authentihash(PVOID fileData, ULONG fileSize, UCHAR output[HASH_SIZE_SHA256]) {
    BCRYPT_ALG_HANDLE        hAlg = NULL;
    BCRYPT_HASH_HANDLE       hHash = NULL;
    NTSTATUS                 status = STATUS_SUCCESS;
    PIMAGE_DOS_HEADER        dosHeader = NULL;
    PIMAGE_NT_HEADERS64      ntHeaders = NULL;
    PIMAGE_OPTIONAL_HEADER64 optHeader = NULL;
    PIMAGE_SECTION_HEADER    sortedSections = NULL;
    ULONG                    checksumOffset = 0;
    ULONG                    secDirOffset = 0;
    ULONG                    afterChecksum = 0;
    ULONG                    afterSecDir = 0;
    ULONG                    headerSize = 0;
    ULONG                    fileEndOffset = 0;
    ULONG                    minOptHeaderSize = 0;
    ULONGLONG                sumOfBytesHashed = 0;
    ULONGLONG                firstSectionOffset = 0;
    USHORT                   numSections = 0;
    SIZE_T                   sectionsBytes = 0;
    BOOLEAN                  hasSecDir = FALSE;
    IMAGE_DATA_DIRECTORY     secDir = {};

    __try {
        if (fileSize < sizeof(IMAGE_DOS_HEADER)) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        dosHeader = (PIMAGE_DOS_HEADER)fileData;
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        if (dosHeader->e_lfanew < (LONG)sizeof(IMAGE_DOS_HEADER) ||
            (ULONGLONG)dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > fileSize) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        ntHeaders = (PIMAGE_NT_HEADERS64)((PUCHAR)fileData + dosHeader->e_lfanew);

        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        // 64-bit images only
        if (ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        optHeader = &ntHeaders->OptionalHeader;

        checksumOffset = (ULONG)dosHeader->e_lfanew
            + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader)
            + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER64, CheckSum);
        afterChecksum = checksumOffset + sizeof(ULONG);

        headerSize = optHeader->SizeOfHeaders;
        if (headerSize > fileSize || headerSize < afterChecksum) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        // The security directory entry may not exist in small optional headers
        minOptHeaderSize = FIELD_OFFSET(IMAGE_OPTIONAL_HEADER64, DataDirectory)
            + (IMAGE_DIRECTORY_ENTRY_SECURITY + 1) * sizeof(IMAGE_DATA_DIRECTORY);

        hasSecDir = optHeader->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_SECURITY &&
                    ntHeaders->FileHeader.SizeOfOptionalHeader >= minOptHeaderSize;

        if (hasSecDir) {
            secDirOffset = (ULONG)dosHeader->e_lfanew
                + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader)
                + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER64, DataDirectory)
                + IMAGE_DIRECTORY_ENTRY_SECURITY * sizeof(IMAGE_DATA_DIRECTORY);
            afterSecDir = secDirOffset + (ULONG)sizeof(IMAGE_DATA_DIRECTORY);

            if (headerSize < afterSecDir) {
                status = STATUS_INVALID_IMAGE_FORMAT;
                goto cleanup;
            }
            secDir = optHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        }

        // Validate section headers before touching them
        numSections = ntHeaders->FileHeader.NumberOfSections;
        if (numSections == 0 || numSections > MAX_SECTIONS) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        sectionsBytes = (SIZE_T)numSections * sizeof(IMAGE_SECTION_HEADER);
        firstSectionOffset = (ULONGLONG)dosHeader->e_lfanew
            + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader)
            + ntHeaders->FileHeader.SizeOfOptionalHeader;

        if (firstSectionOffset > fileSize || sectionsBytes > fileSize - firstSectionOffset) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        // Validate certificate table location
        fileEndOffset = fileSize;
        if (secDir.VirtualAddress != 0 && secDir.Size != 0) {
            if (secDir.VirtualAddress > fileSize || secDir.Size > fileSize - secDir.VirtualAddress) {
                status = STATUS_INVALID_IMAGE_FORMAT;
                goto cleanup;
            }
            fileEndOffset = secDir.VirtualAddress;
        }

        sortedSections = (PIMAGE_SECTION_HEADER)ExAllocatePool2(
            POOL_FLAG_PAGED, sectionsBytes, SECTIONS_TAG);
        if (!sortedSections) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        RtlCopyMemory(sortedSections, (PUCHAR)fileData + firstSectionOffset, sectionsBytes);

        // Insertion sort by PointerToRawData
        for (LONG i = 1; i < (LONG)numSections; i++) {
            IMAGE_SECTION_HEADER tmp = sortedSections[i];
            LONG j = i - 1;
            while (j >= 0 && sortedSections[j].PointerToRawData > tmp.PointerToRawData) {
                sortedSections[j + 1] = sortedSections[j];
                j--;
            }
            sortedSections[j + 1] = tmp;
        }

        status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
        if (!NT_SUCCESS(status))
            goto cleanup;

        status = BCryptCreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0);
        if (!NT_SUCCESS(status))
            goto cleanup;

        // Headers: skip CheckSum and the security directory entry
        status = hash_chunk(hHash, fileData, checksumOffset);
        if (!NT_SUCCESS(status))
            goto cleanup;

        if (hasSecDir) {
            status = hash_chunk(hHash, (PUCHAR)fileData + afterChecksum, secDirOffset - afterChecksum);
            if (!NT_SUCCESS(status))
                goto cleanup;

            status = hash_chunk(hHash, (PUCHAR)fileData + afterSecDir, headerSize - afterSecDir);
            if (!NT_SUCCESS(status))
                goto cleanup;
        }
        else {
            status = hash_chunk(hHash, (PUCHAR)fileData + afterChecksum, headerSize - afterChecksum);
            if (!NT_SUCCESS(status))
                goto cleanup;
        }

        sumOfBytesHashed = headerSize;

        // Sections in file order
        for (USHORT i = 0; i < numSections; i++) {
            ULONG rawOffset = sortedSections[i].PointerToRawData;
            ULONG rawSize = sortedSections[i].SizeOfRawData;

            if (rawSize == 0)
                continue;

            if ((ULONGLONG)rawOffset + rawSize > fileSize) {
                status = STATUS_INVALID_IMAGE_FORMAT;
                goto cleanup;
            }

            status = hash_chunk(hHash, (PUCHAR)fileData + rawOffset, rawSize);
            if (!NT_SUCCESS(status))
                goto cleanup;

            sumOfBytesHashed += rawSize;
        }

        // Extra data after the last section, up to the certificate table
        if (sumOfBytesHashed < fileEndOffset) {
            status = hash_chunk(hHash,
                (PUCHAR)fileData + (ULONG)sumOfBytesHashed,
                fileEndOffset - (ULONG)sumOfBytesHashed);
            if (!NT_SUCCESS(status))
                goto cleanup;
        }

        status = BCryptFinishHash(hHash, output, HASH_SIZE_SHA256, 0);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        status = STATUS_ACCESS_VIOLATION;
    }

cleanup:
    if (sortedSections) ExFreePoolWithTag(sortedSections, SECTIONS_TAG);
    if (hHash)          BCryptDestroyHash(hHash);
    if (hAlg)           BCryptCloseAlgorithmProvider(hAlg, 0);
    return status;
}

NTSTATUS compute_driver_hashes(PUNICODE_STRING FullImageName, PDRIVER_HASHES outHashes) {
    PVOID    fileBuffer = NULL;
    ULONG    fileSize = 0;
    NTSTATUS status;

    if (!FullImageName || !outHashes)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(outHashes, sizeof(DRIVER_HASHES));

    status = read_file_from_disk(FullImageName, &fileBuffer, &fileSize);
    if (!NT_SUCCESS(status))
        return status;

    status = compute_sha256(fileBuffer, fileSize, outHashes->SHA256);
    outHashes->SHA256Valid = NT_SUCCESS(status);

    status = compute_authentihash(fileBuffer, fileSize, outHashes->Authentihash);
    outHashes->AuthentihashValid = NT_SUCCESS(status);

    ExFreePoolWithTag(fileBuffer, DRIVER_FILE_TAG);

    return (outHashes->SHA256Valid || outHashes->AuthentihashValid)
        ? STATUS_SUCCESS
        : STATUS_UNSUCCESSFUL;
}