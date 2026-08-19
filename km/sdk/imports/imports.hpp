#include <ntddk.h>

extern "C" NTSTATUS ZwQuerySystemInformation(
    IN ULONG SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);