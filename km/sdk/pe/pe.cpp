#include "pe.hpp"

// Minimal vector, no STL in kernel


template <typename T>
void vector_free(Vector<T>* v)
{
    if (v->data != NULL) {
        ExFreePoolWithTag(v->data, PE_TAG);
    }
    v->data = NULL;
    v->count = 0;
}

// Allocates a zeroed array of count elements. Old contents are freed.
template <typename T>
BOOLEAN vector_resize(Vector<T>* v, SIZE_T count)
{
    vector_free(v);
    if (count == 0) {
        return TRUE;
    }
    if (count > ((SIZE_T)-1) / sizeof(T)) {
        return FALSE;
    }
    v->data = (T*)ExAllocatePool2(POOL_FLAG_PAGED, count * sizeof(T), PE_TAG);
    if (v->data == NULL) {
        return FALSE;
    }
    v->count = count;
    return TRUE;
}

void free_pe(PE* pe)
{
    for (SIZE_T i = 0; i < pe->sections.count; i++) {
        vector_free(&pe->sections.data[i].data);
    }
    vector_free(&pe->sections);
    pe->valid = FALSE;
}

// Parses dos, nt and section headers from a buffer that starts at the file start / image base.
// Allocates pe->sections (headers only, data is empty).
static BOOLEAN parse_headers(const UCHAR* buf, SIZE_T size, PE* pe)
{
    if (size < sizeof(IMAGE_DOS_HEADER)) {
        return FALSE;
    }
    RtlCopyMemory(&pe->dos, buf, sizeof(IMAGE_DOS_HEADER));
    if (pe->dos.e_magic != IMAGE_DOS_SIGNATURE) {
        return FALSE;
    }

    if (pe->dos.e_lfanew < (LONG)sizeof(IMAGE_DOS_HEADER)) {
        return FALSE;
    }
    SIZE_T nt_off = (SIZE_T)pe->dos.e_lfanew;
    if (nt_off > size || size - nt_off < sizeof(IMAGE_NT_HEADERS64)) {
        return FALSE;
    }
    RtlCopyMemory(&pe->nt, buf + nt_off, sizeof(IMAGE_NT_HEADERS64));
    if (pe->nt.Signature != IMAGE_NT_SIGNATURE) {
        return FALSE;
    }
    if (pe->nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        return FALSE;
    }

    SIZE_T count = pe->nt.FileHeader.NumberOfSections;
    SIZE_T sec_off = nt_off + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) + pe->nt.FileHeader.SizeOfOptionalHeader;
    if (count > 96 || sec_off > size) {
        return FALSE;
    }
    if ((size - sec_off) / sizeof(IMAGE_SECTION_HEADER) < count) {
        return FALSE;
    }

    if (!vector_resize(&pe->sections, count)) {
        return FALSE;
    }
    for (SIZE_T i = 0; i < count; i++) {
        RtlCopyMemory(&pe->sections.data[i].header, buf + sec_off + i * sizeof(IMAGE_SECTION_HEADER), sizeof(IMAGE_SECTION_HEADER));
    }
    return TRUE;
}

// Reads a PE64 from a file. Section data is the raw file data (SizeOfRawData bytes).
// path is an NT path, for example L"\\??\\C:\\Windows\\System32\\ntdll.dll". Must run at PASSIVE_LEVEL.
PE read_pe_from_disk(PUNICODE_STRING path)
{
    PE pe;
    RtlZeroMemory(&pe, sizeof(pe));

    OBJECT_ATTRIBUTES attributes;
    InitializeObjectAttributes(&attributes, path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    HANDLE file = NULL;
    IO_STATUS_BLOCK io;
    NTSTATUS status = ZwCreateFile(&file, GENERIC_READ | SYNCHRONIZE, &attributes, &io, NULL,
                                   FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN,
                                   FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE, NULL, 0);
    if (!NT_SUCCESS(status)) {
        return pe;
    }

    FILE_STANDARD_INFORMATION info;
    status = ZwQueryInformationFile(file, &io, &info, sizeof(info), FileStandardInformation);
    if (!NT_SUCCESS(status) || info.EndOfFile.QuadPart <= 0 || info.EndOfFile.QuadPart > PE_MAX_FILE_SIZE) {
        ZwClose(file);
        return pe;
    }
    ULONG file_size = (ULONG)info.EndOfFile.QuadPart;

    Vector<UCHAR> buf = {};
    if (!vector_resize(&buf, file_size)) {
        ZwClose(file);
        return pe;
    }

    status = ZwReadFile(file, NULL, NULL, NULL, &io, buf.data, file_size, NULL, NULL);
    ZwClose(file);
    if (!NT_SUCCESS(status) || io.Information != file_size) {
        vector_free(&buf);
        return pe;
    }

    if (!parse_headers(buf.data, buf.count, &pe)) {
        free_pe(&pe);
        vector_free(&buf);
        return pe;
    }

    for (SIZE_T i = 0; i < pe.sections.count; i++) {
        SECTION* section = &pe.sections.data[i];
        ULONG offset = section->header.PointerToRawData;
        ULONG raw_size = section->header.SizeOfRawData;

        if (raw_size == 0 || offset >= file_size) {
            continue;
        }
        if (raw_size > file_size - offset) {
            raw_size = file_size - offset;
        }
        if (!vector_resize(&section->data, raw_size)) {
            free_pe(&pe);
            vector_free(&buf);
            return pe;
        }
        RtlCopyMemory(section->data.data, buf.data + offset, raw_size);
    }

    vector_free(&buf);
    pe.valid = TRUE;
    return pe;
}

static BOOLEAN read_process_memory(PEPROCESS process, PVOID address, PVOID out, SIZE_T size)
{
    SIZE_T copied = 0;
    NTSTATUS status = MmCopyVirtualMemory(process, address, PsGetCurrentProcess(), out, size, KernelMode, &copied);
    return NT_SUCCESS(status) && copied == size;
}

// Reads a PE64 module from a running process. base is the module base address in that process.
// Section data is the in-memory data (VirtualSize bytes), as currently mapped.
// Must run at PASSIVE_LEVEL.
PE read_pe_from_pid(HANDLE pid, PVOID base)
{
    PE pe;
    RtlZeroMemory(&pe, sizeof(pe));

    PEPROCESS process = NULL;
    NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
    if (!NT_SUCCESS(status)) {
        return pe;
    }
    if (PsGetProcessExitStatus(process) != STATUS_PENDING) {
        ObDereferenceObject(process);
        return pe;
    }

    Vector<UCHAR> header_buf = {};
    if (!vector_resize(&header_buf, PE_HEADER_READ_SIZE)) {
        ObDereferenceObject(process);
        return pe;
    }
    if (!read_process_memory(process, base, header_buf.data, header_buf.count)) {
        vector_free(&header_buf);
        ObDereferenceObject(process);
        return pe;
    }

    BOOLEAN parsed = parse_headers(header_buf.data, header_buf.count, &pe);
    vector_free(&header_buf);
    if (!parsed) {
        free_pe(&pe);
        ObDereferenceObject(process);
        return pe;
    }

    for (SIZE_T i = 0; i < pe.sections.count; i++) {
        SECTION* section = &pe.sections.data[i];
        ULONG size = section->header.Misc.VirtualSize;
        if (size == 0) {
            size = section->header.SizeOfRawData;
        }
        if (size == 0) {
            continue;
        }

        ULONG rva = section->header.VirtualAddress;
        if (size > PE_MAX_SECTION_SIZE || rva >= pe.nt.OptionalHeader.SizeOfImage ||
            size > pe.nt.OptionalHeader.SizeOfImage - rva) {
            free_pe(&pe);
            ObDereferenceObject(process);
            return pe;
        }

        if (!vector_resize(&section->data, size) ||
            !read_process_memory(process, (PUCHAR)base + rva, section->data.data, size)) {
            free_pe(&pe);
            ObDereferenceObject(process);
            return pe;
        }
    }

    ObDereferenceObject(process);
    pe.valid = TRUE;
    return pe;
}