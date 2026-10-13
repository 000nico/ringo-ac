#include "sdk/sdk.hpp"
#include "tpm.hpp"

static NTSTATUS open_tpm_device(HANDLE* out_handle) {
    UNICODE_STRING device_name = RTL_CONSTANT_STRING(L"\\Device\\TPM");
    OBJECT_ATTRIBUTES obj_attr;
    IO_STATUS_BLOCK io_status;

    InitializeObjectAttributes(&obj_attr, &device_name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    return ZwCreateFile(
        out_handle,
        GENERIC_READ | GENERIC_WRITE,
        &obj_attr,
        &io_status,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
    );
}

static NTSTATUS send_tpm_command(HANDLE h_tpm, UCHAR* command, ULONG command_size, UCHAR* response, ULONG* response_size) {
    IO_STATUS_BLOCK io_status;

    NTSTATUS status = ZwWriteFile(
        h_tpm,
        NULL, NULL, NULL,
        &io_status,
        command,
        command_size,
        NULL, NULL
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlZeroMemory(&io_status, sizeof(io_status));

    status = ZwReadFile(
        h_tpm,
        NULL, NULL, NULL,
        &io_status,
        response,
        *response_size,
        NULL, NULL
    );

    if (NT_SUCCESS(status)) {
        *response_size = (ULONG)io_status.Information;
    }

    return status;
}

TPM_CHECK get_tpm_version() {
    TPM_CHECK tpm = {};

    HANDLE h_tpm = NULL;
    NTSTATUS status = open_tpm_device(&h_tpm);

    if (!NT_SUCCESS(status)) {
        KdPrint(("[-] Failed to open TPM device: 0x%X\n", status));
        return tpm;
    }

    // try TPM 2.0 first — TPM2_GetRandom command
    UCHAR tpm2_command[] = {
        0x80, 0x01,
        0x00, 0x00, 0x00, 0x0C,
        0x00, 0x00, 0x01, 0x7B,
        0x00, 0x20
    };

    UCHAR tpm_response[128] = { 0 };
    ULONG response_length = sizeof(tpm_response);

    status = send_tpm_command(h_tpm, tpm2_command, sizeof(tpm2_command), tpm_response, &response_length);

    if (NT_SUCCESS(status)) {
        if (response_length < 10) {
            KdPrint(("[-] TPM 2.0 response too short (%lu bytes).\n", response_length));
        }
        else {
            UINT32 return_code = (tpm_response[6] << 24) | (tpm_response[7] << 16) |
                (tpm_response[8] << 8) | tpm_response[9];

            tpm.tpm_2_0.valid = TRUE;
            tpm.tpm_2_0.value = return_code == 0;
            if (tpm.tpm_2_0.value) {
                KdPrint(("[+] TPM 2.0 detected and responsive.\n"));
                ZwClose(h_tpm);
                return tpm;
            }
        }
    }

    // fallback: TPM 1.2 — TPM_ORD_GetRandom command
    RtlZeroMemory(tpm_response, sizeof(tpm_response));
    response_length = sizeof(tpm_response);

    UCHAR tpm1_command[] = {
        0x00, 0xC1,
        0x00, 0x00, 0x00, 0x0E,
        0x00, 0x00, 0x00, 0x46,
        0x00, 0x00, 0x00, 0x20
    };

    status = send_tpm_command(h_tpm, tpm1_command, sizeof(tpm1_command), tpm_response, &response_length);

    if (NT_SUCCESS(status)) {
        if (response_length < 10) {
            KdPrint(("[-] TPM 1.2 response too short (%lu bytes).\n", response_length));
        }
        else {
            UINT32 return_code = (tpm_response[6] << 24) | (tpm_response[7] << 16) |
                (tpm_response[8] << 8) | tpm_response[9];

            tpm.tpm_1_2.valid = TRUE;
            tpm.tpm_1_2.value = return_code == 0;
            if (tpm.tpm_1_2.value)
                KdPrint(("[+] TPM 1.2 detected and responsive.\n"));
        }
    }

    KdPrint(("[-] No functional TPM found.\n"));
    ZwClose(h_tpm);
    return tpm;
}