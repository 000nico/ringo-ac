#include "hash_sections.hpp"
#include "core/global.hpp"
#include "sdk/pe/pe.hpp"

static ULONG common_section_size(SECTION* section) {
    ULONG size = section->header.SizeOfRawData;
    ULONG virtual_size = section->header.Misc.VirtualSize;

    if (virtual_size != 0 && virtual_size < size)
        size = virtual_size;
    if (size > section->data.count)
        size = (ULONG)section->data.count;
    return size;
}

// the first hash is done to the image in the disk
bool hash_disk_game_image(UCHAR sha256[HASH_SIZE_SHA256]) {
    PEPROCESS       process = NULL;
    PUNICODE_STRING image_path = NULL;
    PE              pe = {};
    SECTION*        text = NULL;
    ULONG           size = 0;
    bool            result = false;
    NTSTATUS        status;

    status = PsLookupProcessByProcessId(g_ringo_context.Game.protected_game_pid, &process);
    if (!NT_SUCCESS(status))
        return false;

    status = SeLocateProcessImageName(process, &image_path);
    if (!NT_SUCCESS(status))
        goto cleanup;

    pe = read_pe_from_disk(image_path);
    if (!pe.valid)
        goto cleanup;

    text = find_section_by_name(&pe, ".text");
    if (!text)
        goto cleanup;

    size = common_section_size(text);
    if (size == 0)
        goto cleanup;

    status = compute_sha256(text->data.data, size, sha256);
    result = NT_SUCCESS(status);

cleanup:
    free_pe(&pe);
    if (image_path)
        ExFreePool(image_path);
    ObDereferenceObject(process);
    return result;
}

bool hash_process_image(UCHAR sha256[HASH_SIZE_SHA256]) {
    PEPROCESS process = NULL;
    PVOID     base = NULL;
    PE        pe = {};
    SECTION*  text = NULL;
    ULONG     size = 0;
    bool      result = false;
    NTSTATUS  status;

    status = PsLookupProcessByProcessId(g_ringo_context.Game.protected_game_pid, &process);
    if (!NT_SUCCESS(status))
        return false;

    base = PsGetProcessSectionBaseAddress(process);
    if (!base)
        goto cleanup;

    pe = read_pe_from_pid(g_ringo_context.Game.protected_game_pid, base);
    if (!pe.valid)
        goto cleanup;

    text = find_section_by_name(&pe, ".text");
    if (!text)
        goto cleanup;

    size = common_section_size(text);
    if (size == 0)
        goto cleanup;

    status = compute_sha256(text->data.data, size, sha256);
    result = NT_SUCCESS(status);

cleanup:
    free_pe(&pe);
    ObDereferenceObject(process);
    return result;
}

// true if equal
bool compare_hash(const UCHAR hash_1[HASH_SIZE_SHA256], const UCHAR hash_2[HASH_SIZE_SHA256]) {
    return RtlCompareMemory(hash_1, hash_2, HASH_SIZE_SHA256) == HASH_SIZE_SHA256;
}