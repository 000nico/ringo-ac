#include "../sdk.hpp"
#include <ntimage.h>

static NTSTATUS read_file_from_disk(PUNICODE_STRING filePath, PVOID* outBuffer, ULONG* outSize) {
    OBJECT_ATTRIBUTES         objAttr;
    IO_STATUS_BLOCK           ioStatus;
    HANDLE                    fileHandle = NULL;
    NTSTATUS                  status = STATUS_SUCCESS;
    FILE_STANDARD_INFORMATION fileInfo = {};
    PVOID                     buffer = NULL;
    ULONG                     fileSize = 0;
    LARGE_INTEGER             byteOffset = {};

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
        FILE_SHARE_READ,
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

    if (fileInfo.EndOfFile.QuadPart == 0 || fileInfo.EndOfFile.QuadPart > 0x10000000) {
        status = STATUS_FILE_TOO_LARGE;
        goto cleanup;
    }

    fileSize = (ULONG)fileInfo.EndOfFile.QuadPart;

    buffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, fileSize, 'hsrD');
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
        ExFreePoolWithTag(buffer, 'hsrD');
        goto cleanup;
    }

    *outBuffer = buffer;
    *outSize = fileSize;

cleanup:
    ZwClose(fileHandle);
    return status;
}

static NTSTATUS compute_sha256(PVOID data, ULONG dataSize, UCHAR output[HASH_SIZE_SHA256]) {
    BCRYPT_ALG_HANDLE  hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    NTSTATUS           status;

    status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if (!NT_SUCCESS(status))
        return status;

    status = BCryptCreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0);
    if (!NT_SUCCESS(status))
        goto cleanup;

    status = BCryptHashData(hHash, (PUCHAR)data, dataSize, 0);
    if (!NT_SUCCESS(status))
        goto cleanup;

    status = BCryptFinishHash(hHash, output, HASH_SIZE_SHA256, 0);

cleanup:
    if (hHash) BCryptDestroyHash(hHash);
    if (hAlg)  BCryptCloseAlgorithmProvider(hAlg, 0);
    return status;
}

static NTSTATUS compute_authentihash(PVOID fileData, ULONG fileSize, UCHAR output[HASH_SIZE_SHA256]) {
    BCRYPT_ALG_HANDLE     hAlg = NULL;
    BCRYPT_HASH_HANDLE    hHash = NULL;
    NTSTATUS              status = STATUS_SUCCESS;
    PIMAGE_DOS_HEADER     dosHeader = NULL;
    PIMAGE_NT_HEADERS64   ntHeaders = NULL;
    PIMAGE_OPTIONAL_HEADER64 optHeader = NULL;
    PIMAGE_SECTION_HEADER sortedSections = NULL;
    ULONG checksumOffset = 0;
    ULONG secDirOffset = 0;
    ULONG afterChecksum = 0;
    ULONG afterSecDir = 0;
    ULONG headerSize = 0;
    ULONG sumOfBytesHashed = 0;
    ULONG fileEndOffset = 0;
    USHORT numSections = 0;
    SIZE_T sectionsBytes = 0;
    IMAGE_DATA_DIRECTORY secDir = {};

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

        if ((ULONG)dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > fileSize) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        ntHeaders = (PIMAGE_NT_HEADERS64)((PUCHAR)fileData + dosHeader->e_lfanew);

        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        // Solo 64-bit por ahora (la mayoria de cheats son 64-bit)
        if (ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        optHeader = &ntHeaders->OptionalHeader;

        checksumOffset = (ULONG)dosHeader->e_lfanew
            + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader)
            + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER64, CheckSum);

        secDirOffset = (ULONG)dosHeader->e_lfanew
            + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader)
            + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER64, DataDirectory)
            + IMAGE_DIRECTORY_ENTRY_SECURITY * sizeof(IMAGE_DATA_DIRECTORY);

        if (secDirOffset + sizeof(IMAGE_DATA_DIRECTORY) > fileSize) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        secDir = optHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        headerSize = optHeader->SizeOfHeaders;

        if (headerSize > fileSize) {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto cleanup;
        }

        status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
        if (!NT_SUCCESS(status))
            goto cleanup;

        status = BCryptCreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0);
        if (!NT_SUCCESS(status))
            goto cleanup;

        BCryptHashData(hHash, (PUCHAR)fileData, checksumOffset, 0);

        afterChecksum = checksumOffset + 4;
        if (secDirOffset > afterChecksum) {
            BCryptHashData(hHash,
                (PUCHAR)fileData + afterChecksum,
                secDirOffset - afterChecksum,
                0);
        }

        afterSecDir = secDirOffset + (ULONG)sizeof(IMAGE_DATA_DIRECTORY);
        if (headerSize > afterSecDir) {
            BCryptHashData(hHash,
                (PUCHAR)fileData + afterSecDir,
                headerSize - afterSecDir,
                0);
        }

        numSections = ntHeaders->FileHeader.NumberOfSections;
        sectionsBytes = (SIZE_T)numSections * sizeof(IMAGE_SECTION_HEADER);

        sortedSections = (PIMAGE_SECTION_HEADER)ExAllocatePool2(
            POOL_FLAG_NON_PAGED, sectionsBytes, 'ceSd');

        if (!sortedSections) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        RtlCopyMemory(sortedSections, IMAGE_FIRST_SECTION(ntHeaders), sectionsBytes);

        // Insertion sort por PointerToRawData
        for (USHORT i = 1; i < numSections; i++) {
            IMAGE_SECTION_HEADER tmp = sortedSections[i];
            SHORT j = (SHORT)(i - 1);
            while (j >= 0 && sortedSections[j].PointerToRawData > tmp.PointerToRawData) {
                sortedSections[j + 1] = sortedSections[j];
                j--;
            }
            sortedSections[j + 1] = tmp;
        }

        sumOfBytesHashed = headerSize;

        for (USHORT i = 0; i < numSections; i++) {
            ULONG rawOffset = sortedSections[i].PointerToRawData;
            ULONG rawSize = sortedSections[i].SizeOfRawData;

            if (rawSize == 0)
                continue;

            if ((ULONGLONG)rawOffset + rawSize > fileSize)
                continue;

            BCryptHashData(hHash, (PUCHAR)fileData + rawOffset, rawSize, 0);
            sumOfBytesHashed += rawSize;
        }

        fileEndOffset = fileSize;
        if (secDir.VirtualAddress != 0 && secDir.Size != 0)
            fileEndOffset = secDir.VirtualAddress;

        if (sumOfBytesHashed < fileEndOffset) {
            BCryptHashData(hHash,
                (PUCHAR)fileData + sumOfBytesHashed,
                fileEndOffset - sumOfBytesHashed,
                0);
        }

        status = BCryptFinishHash(hHash, output, HASH_SIZE_SHA256, 0);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        status = STATUS_ACCESS_VIOLATION;
    }

cleanup:
    if (sortedSections) ExFreePoolWithTag(sortedSections, 'ceSd');
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

    ExFreePoolWithTag(fileBuffer, 'hsrD');

    return (outHashes->SHA256Valid || outHashes->AuthentihashValid)
        ? STATUS_SUCCESS
        : STATUS_UNSUCCESSFUL;
}