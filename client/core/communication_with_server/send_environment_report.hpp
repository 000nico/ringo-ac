#include "../../../driver/core/system/system.hpp"
#include "core/communication_with_driver/communication_with_driver.hpp"
#include "core/system/hvci/hvci.hpp"
#include "core/system/hypervisor/hypervisor.hpp"
#include "core/system/kernel_debugging/kernel_debugging.hpp"
#include "core/system/secure_boot/secure_boot.hpp"
#include "core/system/test_signing/test_signing.hpp"
#include "core/system/tpm/tpm.hpp"
#include "core/system/vbs/vbs.hpp"
#include "core/identity/identity.hpp"

struct ENVIRONMENT_DATA {
    SECURE_BOOT_CHECK secure_boot;
    HVCI_CHECK hvci;
    VBS_CHECK vbs;
    HYPERVISOR_CHECK hypervisor;
    KERNEL_DEBUGGING_CHECK kernel_debugging;
    TEST_SIGNING_CHECK test_signing;
    TPM_CHECK tpm;
    HWID hwid;
};