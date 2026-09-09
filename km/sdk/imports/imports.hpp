#pragma once
#include <ntifs.h>

#pragma warning(push)
#pragma warning(disable: 4005)
#include <bcrypt.h>
#pragma warning(pop)


extern "C" NTSTATUS ZwQuerySystemInformation(
    IN ULONG SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

extern "C" NTSTATUS NTAPI ZwQueryInformationThread(
    _In_      HANDLE          ThreadHandle,
    _In_      THREADINFOCLASS ThreadInformationClass,
    _Out_     PVOID           ThreadInformation,
    _In_      ULONG           ThreadInformationLength,
    _Out_opt_ PULONG          ReturnLength
);

extern "C" NTKERNELAPI PUCHAR PsGetProcessImageFileName(
    _In_ PEPROCESS Process
);

extern "C" NTKERNELAPI HANDLE PsGetThreadProcessId(
    _In_ PETHREAD Thread
);

extern "C" {
    NTKERNELAPI NTSTATUS ObReferenceObjectByName(
        _In_        PUNICODE_STRING ObjectName,
        _In_        ULONG Attributes,
        _In_opt_    PACCESS_STATE AccessState,
        _In_opt_    ACCESS_MASK DesiredAccess,
        _In_        POBJECT_TYPE ObjectType,
        _In_        KPROCESSOR_MODE AccessMode,
        _Inout_opt_ PVOID ParseContext,
        _Out_       PVOID* Object
    );

    NTKERNELAPI extern POBJECT_TYPE* IoDriverObjectType;
    NTKERNELAPI extern POBJECT_TYPE* IoDeviceObjectType;
}