#include "../../../sdk/imports/imports.hpp"
#include "tpm.hpp"

static NTSTATUS OpenTpmDevice(HANDLE* outHandle) {
    UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\TPM");
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK ioStatus;

    InitializeObjectAttributes(&objAttr, &deviceName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    return ZwCreateFile(
        outHandle,
        GENERIC_READ | GENERIC_WRITE,
        &objAttr,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
    );
}

static NTSTATUS SendTpmCommand(HANDLE hTpm, UCHAR* command, ULONG commandSize, UCHAR* response, ULONG* responseSize) {
    IO_STATUS_BLOCK ioStatus;

    NTSTATUS status = ZwWriteFile(
        hTpm,
        NULL, NULL, NULL,
        &ioStatus,
        command,
        commandSize,
        NULL, NULL
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlZeroMemory(&ioStatus, sizeof(ioStatus));

    status = ZwReadFile(
        hTpm,
        NULL, NULL, NULL,
        &ioStatus,
        response,
        *responseSize,
        NULL, NULL
    );

    if (NT_SUCCESS(status)) {
        *responseSize = (ULONG)ioStatus.Information;
    }

    return status;
}

TPM getTpmVersion() {
    TPM tpm = {};
    tpm.TpmVersion = TpmVersionNone;

    HANDLE hTpm = NULL;
    NTSTATUS status = OpenTpmDevice(&hTpm);

    if (!NT_SUCCESS(status)) {
        KdPrint(("[-] Failed to open TPM device: 0x%X\n", status));
        return tpm;
    }

    // try TPM 2.0 first — TPM2_GetRandom command
    UCHAR tpm2Command[] = {
        0x80, 0x01,
        0x00, 0x00, 0x00, 0x0C,
        0x00, 0x00, 0x01, 0x7B,
        0x00, 0x20
    };

    UCHAR tpmResponse[128] = { 0 };
    ULONG responseLength = sizeof(tpmResponse);

    status = SendTpmCommand(hTpm, tpm2Command, sizeof(tpm2Command), tpmResponse, &responseLength);

    if (NT_SUCCESS(status)) {
        if (responseLength < 10) {
            KdPrint(("[-] TPM 2.0 response too short (%lu bytes).\n", responseLength));
        }
        else {
            UINT32 returnCode = (tpmResponse[6] << 24) | (tpmResponse[7] << 16) |
                (tpmResponse[8] << 8) | tpmResponse[9];

            if (returnCode == 0) {
                KdPrint(("[+] TPM 2.0 detected and responsive.\n"));
                tpm.TpmVersion = TpmVersion2_0;
                ZwClose(hTpm);
                return tpm;
            }
        }
    }

    // fallback: TPM 1.2 — TPM_ORD_GetRandom command
    RtlZeroMemory(tpmResponse, sizeof(tpmResponse));
    responseLength = sizeof(tpmResponse);

    UCHAR tpm1Command[] = {
        0x00, 0xC1,
        0x00, 0x00, 0x00, 0x0E,
        0x00, 0x00, 0x00, 0x46,
        0x00, 0x00, 0x00, 0x20
    };

    status = SendTpmCommand(hTpm, tpm1Command, sizeof(tpm1Command), tpmResponse, &responseLength);

    if (NT_SUCCESS(status)) {
        if (responseLength < 10) {
            KdPrint(("[-] TPM 1.2 response too short (%lu bytes).\n", responseLength));
        }
        else {
            UINT32 returnCode = (tpmResponse[6] << 24) | (tpmResponse[7] << 16) |
                (tpmResponse[8] << 8) | tpmResponse[9];

            if (returnCode == 0) {
                KdPrint(("[+] TPM 1.2 detected and responsive.\n"));
                tpm.TpmVersion = TpmVersion1_2;
                ZwClose(hTpm);
                return tpm;
            }
        }
    }

    KdPrint(("[-] No functional TPM found.\n"));
    ZwClose(hTpm);
    return tpm;
}