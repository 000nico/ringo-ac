#pragma once
#include <ntifs.h>
#include <ntimage.h>

#define PE_TAG 'epEP'

#define PE_MAX_FILE_SIZE 0x10000000
#define PE_MAX_SECTION_SIZE 0x10000000
#define PE_HEADER_READ_SIZE 0x1000

template <typename T>
struct Vector {
    T* data;
    SIZE_T count;
};

struct SECTION {
    IMAGE_SECTION_HEADER header;
    Vector<UCHAR> data;
};

struct PE {
    BOOLEAN valid;
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS64 nt;
    Vector<SECTION> sections;
};

extern "C" NTSTATUS NTAPI MmCopyVirtualMemory(
    PEPROCESS source_process,
    PVOID source_address,
    PEPROCESS target_process,
    PVOID target_address,
    SIZE_T buffer_size,
    KPROCESSOR_MODE previous_mode,
    PSIZE_T return_size);

PE read_pe_from_pid(HANDLE pid, PVOID base);
PE read_pe_from_disk(PUNICODE_STRING path);
SECTION* find_section_by_name(PE* pe, const char* name);
void free_pe(PE* pe);