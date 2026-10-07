#include "protection.hpp"
#include "core/global.hpp"
#include "sdk/crypto/hashes.hpp"
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

// resolves a handle from the real FILE_OBJECT of the mapped image of the process
// this evades a TOCTOU of resolving the path and reopen it
static bool get_process_image_handle(PEPROCESS process, PHANDLE out_handle) {
    PAGED_CODE();

    bool result = false;
    PFILE_OBJECT file_object = NULL;
    OBJECT_ATTRIBUTES obj_attrs;
    HANDLE file_handle = NULL;

    NTSTATUS status = PsReferenceProcessFilePointer(process, &file_object);
    if (!NT_SUCCESS(status))
        return false;

    InitializeObjectAttributes(&obj_attrs, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ObOpenObjectByPointer(
        file_object,
        OBJ_KERNEL_HANDLE,
        NULL,
        FILE_READ_DATA | SYNCHRONIZE,
        *IoFileObjectType,
        KernelMode,
        &file_handle);

    if (NT_SUCCESS(status)) {
        *out_handle = file_handle;
        result = true;
    }

    ObDereferenceObject(file_object);

    return result;
}

bool get_caller_file_hash(PEPROCESS process, UCHAR sha256[HASH_SIZE_SHA256]) {
    bool result = false;
    PUNICODE_STRING image_path = NULL;
    PE pe = { 0 };
    bool pe_loaded = false;

    NTSTATUS status = SeLocateProcessImageName(process, &image_path);
    if (!NT_SUCCESS(status))
        return false;

    pe = read_pe_from_disk(image_path);
    if (!pe.valid)
        goto cleanup;

    pe_loaded = true;

    SECTION* text = find_section_by_name(&pe, ".text");
    if (!text)
        goto cleanup;

    SIZE_T size = common_section_size(text);
    if (size == 0)
        goto cleanup;

    compute_sha256(text->data.data, size, sha256);
    result = true;

cleanup:
    if (pe_loaded)
        free_pe(&pe);
    if (image_path)
        ExFreePool(image_path);

    return result;
}

bool get_caller_process_hash(PEPROCESS process, UCHAR sha256[HASH_SIZE_SHA256]) {
    bool result = false;
    HANDLE file_handle = NULL;
    PE pe = { 0 };
    bool pe_loaded = false;

    if (!get_process_image_handle(process, &file_handle))
        return false;

    pe = read_pe_from_handle(file_handle);
    if (!pe.valid)
        goto cleanup;

    pe_loaded = true;

    SECTION* text = find_section_by_name(&pe, ".text");
    if (!text)
        goto cleanup;

    SIZE_T size = common_section_size(text);
    if (size == 0)
        goto cleanup;

    compute_sha256(text->data.data, size, sha256);
    result = true;

cleanup:
    if (pe_loaded)
        free_pe(&pe);
    if (file_handle)
        ZwClose(file_handle);

    return result;
}

bool constant_time_compare(const UCHAR* a, const UCHAR* b, SIZE_T size)
{
    volatile UCHAR diff = 0;
    for (SIZE_T i = 0; i < size; i++) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

bool compare_sha256(const UCHAR a[HASH_SIZE_SHA256], const UCHAR b[HASH_SIZE_SHA256])
{
    return constant_time_compare(a, b, HASH_SIZE_SHA256);
}

bool is_peprocess_banned(PEPROCESS peprocess) {
    for(int i = 0; i < g_ringo_context.Session.Banned.index; i++) {
        if(g_ringo_context.Session.Banned.banned_peprocess[i] == peprocess) {
            return true;
        }
    }

    return false;
}