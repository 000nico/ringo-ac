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

// Aseguramos la macro si no está cargada desde <winternl.h> / <ntstatus.h>
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

int main()
{
    HANDLE hDriver = NULL;
    if (open_driver(&hDriver))
        std::cout << "[+] Driver opened" << std::endl;
    else
        std::cout << "[-] Error opening driver" << std::endl;

    if (NT_SUCCESS(setGameProcessName(hDriver, "cs2.exe")))
        std::cout << "[+] Game process name set" << std::endl;
    else
        std::cout << "[-] Error setting game name" << std::endl;

    HANDLE pid = NULL;
    DWORD bytesReturned = 0;
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoHwid, &pid, sizeof(pid), &bytesReturned)))
        std::cout << "[+] Game pid = " << pid << std::endl;
    else
        std::cout << "[-] Error getting game pid" << std::endl;

    // Environment Checks
    HVCI_CHECK hvci = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoHvci, &hvci, sizeof(hvci), &bytesReturned)))
        std::cout << "[+] HVCI = " << hvci.isEnabled << std::endl;
    else
        std::cout << "[-] Error getting HVCI info" << std::endl;

    HYPERVISOR_CHECK hypervisor = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoHypervisorStatus, &hypervisor, sizeof(hypervisor), &bytesReturned)))
        std::cout << "[+] Hypervisor = " << hypervisor.hypervisorPresent << std::endl;
    else
        std::cout << "[-] Error getting Hypervisor info" << std::endl;

    KERNEL_DEBUGGING_CHECK kernel_debugging_check = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoKernelDebuggingStatus, &kernel_debugging_check, sizeof(kernel_debugging_check), &bytesReturned)))
        std::cout << "[+] Kernel Debugging = " << kernel_debugging_check.isEnabled << std::endl;
    else
        std::cout << "[-] Error getting Kernel Debugging info" << std::endl;

    SECURE_BOOT_CHECK secure_boot_check = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoSecureBoot, &secure_boot_check, sizeof(secure_boot_check), &bytesReturned)))
        std::cout << "[+] Secure Boot = " << secure_boot_check.isEnabled << std::endl;
    else
        std::cout << "[-] Error getting Secure Boot info" << std::endl;

    TEST_SIGNING_CHECK test_signing_check = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoTestSigningMode, &test_signing_check, sizeof(test_signing_check), &bytesReturned)))
        std::cout << "[+] Test Signing = " << test_signing_check.isEnabled << std::endl;
    else
        std::cout << "[-] Error getting Test Signing info" << std::endl;

    VBS_CHECK vbs_check = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoVbs, &vbs_check, sizeof(vbs_check), &bytesReturned)))
        std::cout << "[+] VBS = " << vbs_check.isVBSEnabled << std::endl;
    else
        std::cout << "[-] Error getting VBS info" << std::endl;

    TPM tpm_check = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoVbs, &tpm_check, sizeof(tpm_check), &bytesReturned)))
        std::cout << "[+] TPM = " << tpm_check.TpmVersion << std::endl;
    else
        std::cout << "[-] Error getting TPM info" << std::endl;

    // Callbacks Status
    BOOLEAN isCreateProcessCallbackSet = FALSE;
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoCreateProcessCallbackSet, &isCreateProcessCallbackSet, sizeof(isCreateProcessCallbackSet), &bytesReturned)))
        std::cout << "[+] CreateProcess Callback Set = " << (isCreateProcessCallbackSet ? "TRUE" : "FALSE") << std::endl;
    else
        std::cout << "[-] Error getting CreateProcess Callback info" << std::endl;

    BOOLEAN isCreateThreadCallbackSet = FALSE;
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoCreateThreadCallbackSet, &isCreateThreadCallbackSet, sizeof(isCreateThreadCallbackSet), &bytesReturned)))
        std::cout << "[+] CreateThread Callback Set = " << (isCreateThreadCallbackSet ? "TRUE" : "FALSE") << std::endl;
    else
        std::cout << "[-] Error getting CreateThread Callback info" << std::endl;

    BOOLEAN isImageCallbackSet = FALSE;
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoImageCallbackSet, &isImageCallbackSet, sizeof(isImageCallbackSet), &bytesReturned)))
        std::cout << "[+] LoadImage Callback Set = " << (isImageCallbackSet ? "TRUE" : "FALSE") << std::endl;
    else
        std::cout << "[-] Error getting LoadImage Callback info" << std::endl;

    BOOLEAN isObCallbackSet = FALSE;
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoObCallbackSet, &isObCallbackSet, sizeof(isObCallbackSet), &bytesReturned)))
        std::cout << "[+] ObRegisterCallbacks Set = " << (isObCallbackSet ? "TRUE" : "FALSE") << std::endl;
    else
        std::cout << "[-] Error getting ObCallback info" << std::endl;

    // telemetry
    HWID hwid = {};
    if (NT_SUCCESS(GetEnvironmentInformation(hDriver, InfoHwid, &hwid, sizeof(hwid), &bytesReturned)))
        std::cout << "[+] HWID[cpuHash = " << hwid.CpuHash << ", diskSerial = " << hwid.DiskSerial << ", hwidSerial = " << hwid.SmbiosUuid << "]" << std::endl;
    else
        std::cout << "[-] Error getting HWID" << std::endl;

    return 0;
}