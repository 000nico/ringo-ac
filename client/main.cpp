#include <iostream>

#include "core/driver.hpp"
#include "core/communication/get_env_info.hpp"
#include "core/communication/communication.hpp"

#include "../driver/core/system/system.hpp"
#include "../driver/core/identity/identity.hpp"


#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

int main()
{
    HANDLE hDriver = NULL;
    if (open_driver(&hDriver))
        std::cout << "[+] Driver opened" << std::endl;
    else {
        std::cout << "[-] Error opening driver" << std::endl;
        return 1;
    }

    DWORD bytesReturned = 0;

    if (NT_SUCCESS(set_game_process_name(hDriver, "cs2.exe")))
        std::cout << "[+] Game process name set" << std::endl;
    else
        std::cout << "[-] Error setting game name" << std::endl;

    // PID — fix InfoHwid → InfoGamePid
    DWORD pid = 0;
    if (NT_SUCCESS(get_environment_information(hDriver, InfoGamePid, &pid, sizeof(pid), &bytesReturned)))
        std::cout << "[+] Game pid = " << pid << std::endl;
    else
        std::cout << "[-] Error getting game pid" << std::endl;

    // Environment checks 
    HVCI_CHECK hvci = {};
    if (NT_SUCCESS(get_environment_information(hDriver, InfoHvci, &hvci, sizeof(hvci), &bytesReturned)))
        std::cout << "[+] HVCI = " << (hvci.code_integrity.valid ? (hvci.code_integrity.value ? "TRUE" : "FALSE") : "INVALID") << std::endl;

    HYPERVISOR_CHECK hypervisor = {};
    if (NT_SUCCESS(get_environment_information(hDriver, InfoHypervisorStatus, &hypervisor, sizeof(hypervisor), &bytesReturned)))
        std::cout << "[+] Hypervisor NT query = " << (hypervisor.nt_query.valid ? (hypervisor.nt_query.value ? "TRUE" : "FALSE") : "INVALID")
            << ", CPUID = " << (hypervisor.cpuid.valid ? (hypervisor.cpuid.value ? "TRUE" : "FALSE") : "INVALID") << std::endl;

    KERNEL_DEBUGGING_CHECK kd = {};
    if (NT_SUCCESS(get_environment_information(hDriver, InfoKernelDebuggingStatus, &kd, sizeof(kd), &bytesReturned)))
        std::cout << "[+] Kernel Debugger enabled = " << (kd.nt_query_enabled.valid ? (kd.nt_query_enabled.value ? "TRUE" : "FALSE") : "INVALID")
            << ", not present = " << (kd.nt_query_not_present.valid ? (kd.nt_query_not_present.value ? "TRUE" : "FALSE") : "INVALID") << std::endl;

    SECURE_BOOT_CHECK sb = {};
    if (NT_SUCCESS(get_environment_information(hDriver, InfoSecureBoot, &sb, sizeof(sb), &bytesReturned)))
        std::cout << "[+] Secure Boot NT query = " << (sb.nt_query_enabled.valid ? (sb.nt_query_enabled.value ? "TRUE" : "FALSE") : "INVALID")
            << ", capable = " << (sb.nt_query_capable.valid ? (sb.nt_query_capable.value ? "TRUE" : "FALSE") : "INVALID")
            << ", UEFI = " << (sb.uefi_var.valid ? (sb.uefi_var.value ? "TRUE" : "FALSE") : "INVALID")
            << ", registry = " << (sb.registry.valid ? (sb.registry.value ? "TRUE" : "FALSE") : "INVALID") << std::endl;

    TEST_SIGNING_CHECK ts = {};
    if (NT_SUCCESS(get_environment_information(hDriver, InfoTestSigningMode, &ts, sizeof(ts), &bytesReturned)))
        std::cout << "[+] Test Signing = " << (ts.code_integrity.valid ? (ts.code_integrity.value ? "TRUE" : "FALSE") : "INVALID") << std::endl;

    VBS_CHECK vbs = {};
    if (NT_SUCCESS(get_environment_information(hDriver, InfoVbs, &vbs, sizeof(vbs), &bytesReturned)))
        std::cout << "[+] VBS = " << (vbs.nt_query_secure_kernel.valid ? (vbs.nt_query_secure_kernel.value ? "TRUE" : "FALSE") : "INVALID")
            << ", HVCI = " << (vbs.nt_query_hvci.valid ? (vbs.nt_query_hvci.value ? "TRUE" : "FALSE") : "INVALID") << std::endl;

    // TPM — fix InfoVbs → InfoTpm
    TPM_CHECK tpm = {};
    if (NT_SUCCESS(get_environment_information(hDriver, InfoTpm, &tpm, sizeof(tpm), &bytesReturned)))
        std::cout << "[+] TPM 2.0 = " << (tpm.tpm_2_0.valid ? (tpm.tpm_2_0.value ? "TRUE" : "FALSE") : "INVALID")
            << ", TPM 1.2 = " << (tpm.tpm_1_2.valid ? (tpm.tpm_1_2.value ? "TRUE" : "FALSE") : "INVALID") << std::endl;

    // Callbacks
    BOOLEAN cb = FALSE;
    if (NT_SUCCESS(get_environment_information(hDriver, InfoCreateProcessCallbackSet, &cb, sizeof(cb), &bytesReturned)))
        std::cout << "[+] CreateProcess Callback = " << (cb ? "TRUE" : "FALSE") << std::endl;
    cb = FALSE;
    if (NT_SUCCESS(get_environment_information(hDriver, InfoCreateThreadCallbackSet, &cb, sizeof(cb), &bytesReturned)))
        std::cout << "[+] CreateThread Callback = " << (cb ? "TRUE" : "FALSE") << std::endl;
    cb = FALSE;
    if (NT_SUCCESS(get_environment_information(hDriver, InfoImageCallbackSet, &cb, sizeof(cb), &bytesReturned)))
        std::cout << "[+] LoadImage Callback = " << (cb ? "TRUE" : "FALSE") << std::endl;
    cb = FALSE;
    if (NT_SUCCESS(get_environment_information(hDriver, InfoObCallbackSet, &cb, sizeof(cb), &bytesReturned)))
        std::cout << "[+] ObRegisterCallbacks = " << (cb ? "TRUE" : "FALSE") << std::endl;

    // HWID 
    HWID hwid = {};
    if (NT_SUCCESS(get_environment_information(hDriver, InfoHwid, &hwid, sizeof(hwid), &bytesReturned))) {
        printf("[+] HWID[cpuHash = %lu, diskSerial = %lu, hwidSerial = ", hwid.CpuHash, hwid.DiskSerial);
        for (int i = 0; i < sizeof(hwid.SmbiosUuid); i++)
            printf("%02X", ((BYTE*)&hwid.SmbiosUuid)[i]);
        printf("]\n");
    }

    return 0;
}