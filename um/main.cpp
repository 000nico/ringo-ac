#include <iostream>

#include "core/driver.hpp"
#include "core/comunication/ioctl/get_env_info.hpp"
#include "core/comunication/ioctl/ioctl.hpp"

#include "../km/core/enviroment/hvci/hvci.hpp"
#include "../km/core/enviroment/hypervisor/hypervisor.hpp"
#include "../km/core/enviroment/kernel_debugging/kernel_debugging.hpp"
#include "../km/core/enviroment/secure_boot/secure_boot.hpp"
#include "../km/core/enviroment/test_signing/test_signing.hpp"
#include "../km/core/enviroment/vbs/vbs.hpp"
#include "../km/core/enviroment/tpm/tpm.hpp"

#include "../km/core/telemetry/hwid/hwid.hpp"

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

    if (NT_SUCCESS(setGameProcessName(hDriver, "cs2.exe")))
        std::cout << "[+] Game process name set" << std::endl;
    else
        std::cout << "[-] Error setting game name" << std::endl;

    // PID — fix InfoHwid → InfoGamePid
    DWORD pid = 0;
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoGamePid, &pid, sizeof(pid), &bytesReturned)))
        std::cout << "[+] Game pid = " << pid << std::endl;
    else
        std::cout << "[-] Error getting game pid" << std::endl;

    // Environment checks — fix BOOLEAN printing
    HVCI_CHECK hvci = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoHvci, &hvci, sizeof(hvci), &bytesReturned)))
        std::cout << "[+] HVCI = " << (hvci.isEnabled ? "TRUE" : "FALSE") << std::endl;

    HYPERVISOR_CHECK hypervisor = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoHypervisorStatus, &hypervisor, sizeof(hypervisor), &bytesReturned)))
        std::cout << "[+] Hypervisor = " << (hypervisor.hypervisorPresent ? "TRUE" : "FALSE") << std::endl;

    KERNEL_DEBUGGING_CHECK kd = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoKernelDebuggingStatus, &kd, sizeof(kd), &bytesReturned)))
        std::cout << "[+] Kernel Debugging = " << (kd.isEnabled ? "TRUE" : "FALSE") << std::endl;

    SECURE_BOOT_CHECK sb = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoSecureBoot, &sb, sizeof(sb), &bytesReturned)))
        std::cout << "[+] Secure Boot = " << (sb.isEnabled ? "TRUE" : "FALSE") << std::endl;

    TEST_SIGNING_CHECK ts = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoTestSigningMode, &ts, sizeof(ts), &bytesReturned)))
        std::cout << "[+] Test Signing = " << (ts.isEnabled ? "TRUE" : "FALSE") << std::endl;

    VBS_CHECK vbs = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoVbs, &vbs, sizeof(vbs), &bytesReturned)))
        std::cout << "[+] VBS = " << (vbs.isVBSEnabled ? "TRUE" : "FALSE") << std::endl;

    // TPM — fix InfoVbs → InfoTpm
    TPM tpm = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoTpm, &tpm, sizeof(tpm), &bytesReturned)))
        std::cout << "[+] TPM = " << tpm.TpmVersion << std::endl;

    // Callbacks
    BOOLEAN cb = FALSE;
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoCreateProcessCallbackSet, &cb, sizeof(cb), &bytesReturned)))
        std::cout << "[+] CreateProcess Callback = " << (cb ? "TRUE" : "FALSE") << std::endl;
    cb = FALSE;
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoCreateThreadCallbackSet, &cb, sizeof(cb), &bytesReturned)))
        std::cout << "[+] CreateThread Callback = " << (cb ? "TRUE" : "FALSE") << std::endl;
    cb = FALSE;
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoImageCallbackSet, &cb, sizeof(cb), &bytesReturned)))
        std::cout << "[+] LoadImage Callback = " << (cb ? "TRUE" : "FALSE") << std::endl;
    cb = FALSE;
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoObCallbackSet, &cb, sizeof(cb), &bytesReturned)))
        std::cout << "[+] ObRegisterCallbacks = " << (cb ? "TRUE" : "FALSE") << std::endl;

    // HWID — fix UUID como hex
    HWID hwid = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoHwid, &hwid, sizeof(hwid), &bytesReturned))) {
        printf("[+] HWID[cpuHash = %lu, diskSerial = %lu, hwidSerial = ",
            hwid.CpuHash, hwid.DiskSerial);
        for (int i = 0; i < sizeof(hwid.SmbiosUuid); i++)
            printf("%02X", ((BYTE*)&hwid.SmbiosUuid)[i]);
        printf("]\n");
    }

    return 0;
}