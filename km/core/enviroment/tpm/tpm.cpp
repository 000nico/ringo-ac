#pragma comment(lib, "tbs.lib")
#include "tpm.hpp"
#include <ntddk.h>
#include <tbs.h>

TPM getTpmVersion() {
    TPM tpm = {};

    TBS_HCONTEXT hContext = NULL;
    TBS_RESULT tbsResult;
    //NTSTATUS status = STATUS_SUCCESS;

    // Attempt to detect TPM 2.0
    TBS_CONTEXT_PARAMS2 contextParams = { 0 };
    contextParams.version = TBS_CONTEXT_VERSION_TWO;
    contextParams.includeTpm12 = 0;
    contextParams.includeTpm20 = 1;

    tbsResult = Tbsi_Context_Create((PCTBS_CONTEXT_PARAMS)&contextParams, &hContext);

    if (tbsResult == TBS_SUCCESS && hContext != NULL) {
        // TPM2_GetRandom
        UCHAR tpm2Command[] = {
            0x80, 0x01,
            0x00, 0x00, 0x00, 0x0C,
            0x00, 0x00, 0x01, 0x7B,
            0x00, 0x20
        };

        UCHAR tpmResponse[128] = { 0 };
        UINT32 responseLength = sizeof(tpmResponse);

        tbsResult = Tbsip_Submit_Command(
            hContext, TBS_COMMAND_LOCALITY_ZERO, TBS_COMMAND_PRIORITY_NORMAL,
            tpm2Command, sizeof(tpm2Command), tpmResponse, &responseLength
        );

        if (tbsResult == TBS_SUCCESS) {
            UINT32 tpmReturnCode = (tpmResponse[6] << 24) | (tpmResponse[7] << 16) |
                (tpmResponse[8] << 8) | (tpmResponse[9]);

            if (tpmReturnCode == 0) {
                KdPrint(("[+] AntiCheat: TPM 2.0 detected and responsive.\n"));

                tpm.TpmVersion = TpmVersion2_0;

                Tbsip_Context_Close(hContext);
                return tpm;
            }
        }

        Tbsip_Context_Close(hContext);
        hContext = NULL;
    }

    // Fallback: Attempt to detect TPM 1.2
    contextParams.includeTpm12 = 1;
    contextParams.includeTpm20 = 0;

    tbsResult = Tbsi_Context_Create((PCTBS_CONTEXT_PARAMS)&contextParams, &hContext);

    if (tbsResult == TBS_SUCCESS && hContext != NULL) {
        // TPM_ORD_GetRandom
        UCHAR tpm1Command[] = {
            0x00, 0xC1,
            0x00, 0x00, 0x00, 0x0E,
            0x00, 0x00, 0x00, 0x46,
            0x00, 0x00, 0x00, 0x20
        };

        UCHAR tpmResponse[128] = { 0 };
        UINT32 responseLength = sizeof(tpmResponse);

        tbsResult = Tbsip_Submit_Command(
            hContext, TBS_COMMAND_LOCALITY_ZERO, TBS_COMMAND_PRIORITY_NORMAL,
            tpm1Command, sizeof(tpm1Command), tpmResponse, &responseLength
        );

        if (tbsResult == TBS_SUCCESS) {
            UINT32 tpmReturnCode = (tpmResponse[6] << 24) | (tpmResponse[7] << 16) |
                (tpmResponse[8] << 8) | (tpmResponse[9]);

            if (tpmReturnCode == 0) {
                KdPrint(("[+] AntiCheat: TPM 1.2 detected and responsive.\n"));

                tpm.TpmVersion = TpmVersion1_2;

                Tbsip_Context_Close(hContext);
                return tpm;
            }
        }

        Tbsip_Context_Close(hContext);
    }

    tpm.TpmVersion = TpmVersionNone;

    KdPrint(("[-] AntiCheat: No functional TPM found on the system.\n"));

    return tpm;
}