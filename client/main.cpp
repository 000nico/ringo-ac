#include <iostream>

#include "core/driver/driver.hpp"
#include "core/scheduler/scheduler.hpp"
#include "core/communication_with_driver/communication_with_driver.hpp"
#include "../driver/core/system/system.hpp"
#include "../driver/core/identity/identity.hpp"

template <typename T>
static const char* str(const T& c) {
    return c.valid ? (c.value ? "TRUE" : "FALSE") : "INVALID";
}

template <typename T>
static bool query(HANDLE drv, ENVIRONMENT_INFO_CLASS cls, T& out) {
    DWORD returned = 0;
    return get_environment_information(drv, cls, &out, sizeof(out), &returned) != FALSE;
}

int main() {
    HANDLE drv = NULL;
    if (!open_driver(&drv)) {
        std::cout << "[-] Error opening driver\n";
        return 1;
    }
    std::cout << "[+] Driver opened\n";

    std::cout << (set_game_process_name(drv, "cs2.exe") >= 0
        ? "[+] Game process name set\n" : "[-] Error setting game name\n");

    HANDLE pid = NULL;
    if (query(drv, INFO_GAME_PID, pid))
        std::cout << "[+] Game pid = " << (ULONG_PTR)pid << "\n";

    HVCI_CHECK hvci = {};
    if (query(drv, INFO_HVCI, hvci))
        std::cout << "[+] HVCI = " << str(hvci.code_integrity) << "\n";

    HYPERVISOR_CHECK hv = {};
    if (query(drv, INFO_HYPERVISOR_STATUS, hv))
        std::cout << "[+] Hypervisor NT query = " << str(hv.nt_query)
                  << ", CPUID = " << str(hv.cpuid) << "\n";

    KERNEL_DEBUGGING_CHECK kd = {};
    if (query(drv, INFO_KERNEL_DEBUGGING_STATUS, kd))
        std::cout << "[+] Kernel Debugger enabled = " << str(kd.nt_query_enabled)
                  << ", not present = " << str(kd.nt_query_not_present) << "\n";

    SECURE_BOOT_CHECK sb = {};
    if (query(drv, INFO_SECURE_BOOT, sb))
        std::cout << "[+] Secure Boot NT query = " << str(sb.nt_query_enabled)
                  << ", capable = " << str(sb.nt_query_capable)
                  << ", UEFI = " << str(sb.uefi_var)
                  << ", registry = " << str(sb.registry) << "\n";

    TEST_SIGNING_CHECK ts = {};
    if (query(drv, INFO_TEST_SIGNING_MODE, ts))
        std::cout << "[+] Test Signing = " << str(ts.code_integrity) << "\n";

    VBS_CHECK vbs = {};
    if (query(drv, INFO_VBS, vbs))
        std::cout << "[+] VBS = " << str(vbs.nt_query_secure_kernel)
                  << ", HVCI = " << str(vbs.nt_query_hvci) << "\n";

    TPM_CHECK tpm = {};
    if (query(drv, INFO_TPM, tpm))
        std::cout << "[+] TPM 2.0 = " << str(tpm.tpm_2_0)
                  << ", TPM 1.2 = " << str(tpm.tpm_1_2) << "\n";

    struct { ENVIRONMENT_INFO_CLASS cls; const char* name; } callbacks[] = {
        { INFO_CREATE_PROCESS_CALLBACK_SET, "CreateProcess Callback" },
        { INFO_CREATE_THREAD_CALLBACK_SET,  "CreateThread Callback"  },
        { INFO_IMAGE_CALLBACK_SET,         "LoadImage Callback"     },
        { INFO_OB_CALLBACK_SET,            "ObRegisterCallbacks"    },
    };
    
    for (auto& c : callbacks) {
        BOOLEAN set = FALSE;
        if (query(drv, c.cls, set))
            std::cout << "[+] " << c.name << " = " << (set ? "TRUE" : "FALSE") << "\n";
    }

    HWID hwid = {};
    if (query(drv, INFO_HWID, hwid)) {
        printf("[+] HWID[cpuHash = %lu, diskSerial = %lu, hwidSerial = ", hwid.cpu_hash, hwid.disk_serial);
        for (size_t i = 0; i < sizeof(hwid.smbios_uuid); i++)
            printf("%02X", ((BYTE*)&hwid.smbios_uuid)[i]);
        printf("]\n");
    }

    std::cout << "[*] scheduler: environment report every ~2 minutes\n";
    run_scheduler(drv);

    return 0;
}